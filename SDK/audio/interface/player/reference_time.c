/*************************************************************************************************/
/*!
*  \file      reference_time.c
*
*  \brief     提供音频参考时钟选择的接口函数实体与管理
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/

#include "reference_time.h"
#include "system/includes.h"

/*
 * 关于优先级选择：
 * 手机的网络时钟优先级最高，无手机网络的情况下选择TWS网络
 */
static LIST_HEAD(reference_head);
static u8 local_network = 0xff;
static u8 local_net_addr[6];
static u8 local_network_id = 1;

struct reference_clock {
    u8 id;
    u8 network;
    u8 net_addr[6];
    struct list_head entry;
};

extern void bt_audio_reference_clock_select(void *addr, u8 network);
extern u32 bt_audio_reference_clock_time(u8 network);
extern u32 bt_audio_reference_clock_remapping(void *src_addr, u8 src_network, void *dst_addr, u8 dst_network, u32 clock);
extern u8 bt_audio_reference_link_exist(void *addr, u8 network);

int audio_reference_clock_select(void *addr, u8 network)
{
    struct reference_clock *reference_clock = (struct reference_clock *)zalloc(sizeof(struct reference_clock));
    struct reference_clock *clk;
    u8 reset_clock = 1;

    local_irq_disable();

    if (network > 2) {
        local_irq_enable();
        return 0;
    }

    if (list_empty(&reference_head) || network == 0) {
        reference_clock->network = network;
        if (addr == NULL) {
            memset(reference_clock->net_addr, 0x0, sizeof(reference_clock->net_addr));
        } else {
            memcpy(reference_clock->net_addr, addr, sizeof(reference_clock->net_addr));
        }
    } else {
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (clk->network == 0 && bt_audio_reference_link_exist(clk->net_addr, clk->network)) {
            reference_clock->network = clk->network;
            memcpy(reference_clock->net_addr, clk->net_addr, sizeof(reference_clock->net_addr));
            reset_clock = 0;
        } else {
            reference_clock->network = network;
            if (addr == NULL) {
                memset(reference_clock->net_addr, 0x0, sizeof(reference_clock->net_addr));
            } else {
                memcpy(reference_clock->net_addr, addr, sizeof(reference_clock->net_addr));
            }
        }
    }

    list_add(&reference_clock->entry, &reference_head);
    reference_clock->id = local_network_id;
    if (++local_network_id == 0) {
        local_network_id++;
    }
    local_irq_enable();

    if (reset_clock) {
        bt_audio_reference_clock_select(reference_clock->net_addr, reference_clock->network);
    }

    return reference_clock->id;
}

u32 audio_reference_clock_time(void)
{
    if (!list_empty(&reference_head)) {
        struct reference_clock *clk;
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        return bt_audio_reference_clock_time(clk->network);
    }

    return bt_audio_reference_clock_time(local_network);
}

u32 audio_reference_network_clock_time(u8 network)
{
    return bt_audio_reference_clock_time(network);
}

u8 audio_reference_network_exist(u8 id)
{
    struct reference_clock *clk;
    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (clk->id == id) {
            goto exist_detect;
        }
    }

    local_irq_enable();
    return 0;

exist_detect:
    local_irq_enable();
    return bt_audio_reference_link_exist(clk->net_addr, clk->network);
}

u8 is_audio_reference_clock_enable(void)
{
    return 0;
    /*return local_network == 0xff ? 0 : 1;*/
}

u8 audio_reference_clock_network(void *addr)
{
    struct reference_clock *clk;

    if (list_empty(&reference_head)) {
        return -1;
    }

    clk = list_first_entry(&reference_head, struct reference_clock, entry);
    if (addr) {
        memcpy(addr, clk->net_addr, 6);
    }

    return clk->network;
}

u8 audio_reference_clock_match(void *addr, u8 network)
{
    struct reference_clock *clk;

    if (list_empty(&reference_head)) {
        return 0;
    }
    clk = list_first_entry(&reference_head, struct reference_clock, entry);
    if (clk->network == network) {
        if (network == 0) {
            if (addr && memcmp(clk->net_addr, addr, 6) == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

u32 audio_reference_clock_remapping(u8 now_network, u8 dst_network, u32 clock)
{
    void *now_addr = NULL;
    void *dst_addr = NULL;
    struct reference_clock *clk;

    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (!now_addr && clk->network == now_network) {
            now_addr = clk->net_addr;
        }
        if (!dst_addr && clk->network == dst_network) {
            dst_addr = clk->net_addr;
        }
    }
    local_irq_enable();

    return bt_audio_reference_clock_remapping(now_addr, now_network, dst_addr, dst_network, clock);
}

void audio_reference_clock_exit(u8 id)
{
    struct reference_clock *clk;
    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (clk->id == id) {
            goto delete;
        }
    }

    local_irq_enable();
    return;
delete:
    list_del(&clk->entry);
    free(clk);
    if (!list_empty(&reference_head)) {
        /*clk = list_first_entry(&reference_head, struct reference_clock, entry);*/
        /*bt_audio_reference_clock_select(clk->net_addr, clk->network);*/
    }
    local_irq_enable();
}
