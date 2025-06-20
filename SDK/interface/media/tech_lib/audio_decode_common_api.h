#ifndef AUDIO_DECODE_COMMON_API_H
#define AUDIO_DECODE_COMMON_API_H

#include "cpu.h"

/********************* DEC *********************/
struct if_decoder_io {
    void *priv ;
    int (*input)(void *priv, u32 addr, void *buf, int len, u8 type);
    /*
    priv -- 私有结构体，由初始化函数提供。
    addr -- 文件位置
    buf  -- 读入地址
    len  -- 读入长度 512 的整数倍
    type -- 0 --同步读（等到数据读回来，函数才返回） ，1 -- 异步读（不等数据读回来，函数就放回）

    */
    int (*check_buf)(void *priv, u32 addr, void *buf);
    int (*output)(void *priv, void *data, int len);
    u32(*get_lslen)(void *priv);
    u32(*store_rev_data)(void *priv, u32 addr, int len);
    int (*input_fr)(void *priv, void **buf);
};

typedef struct if_decoder_io IF_DECODER_IO;

typedef struct decoder_inf {
    u32 sr ;            ///< sample rate
    u32 br ;            ///< bit rate
    u32 nch ;           ///<声道
    u32 total_time;     ///<总时间
} dec_inf_t ;


typedef struct __audio_decoder_ops {
    char *name;                                                            ///< 解码器名称
    u32(*open)(void *work_buf, void *stk_buf, const struct if_decoder_io *decoder_io, u8 *bk_point_ptr, void *dci);  ///<打开解码器

    u32(*format_check)(void *work_buf, void *stk_buf);					///<格式检查

    u32(*run)(void *work_buf, u32 type, void *stk_buf);					///<主循环

    dec_inf_t *(*get_dec_inf)(void *work_buf);				///<获取解码信息
    u32(*get_playtime)(void *work_buf);					///<获取播放时间
    u32(*get_bp_inf)(void *work_buf);						///<获取断点信息;返回断点信息存放的地址

    //u32 (*need_workbuf_size)() ;							///<获取整个解码所需的buffer
    u32(*need_dcbuf_size)(void *dci);						///<获取解码需要的buffer
    u32(*need_skbuf_size)(void *dci); 						///<获取解码过程中stack_buf的大小
    u32(*need_bpbuf_size)(void *dci);						///<获取保存断点信息需要的buffer的长度

    //void (*set_dcbuf)(void* ptr);			                ///<设置解码buffer
    //void (*set_bpbuf)(void *work_buf,void* ptr);			///<设置断点保存buffer

    void (*set_step)(void *work_buf, u32 step);				///<设置快进快进步长。
    void (*set_err_info)(void *work_buf, u32 cmd, u8 *ptr, u32 size);		///<设置解码的错误条件
    u32(*dec_config)(void *work_buf, u32 cmd, void *parm);
} audio_decoder_ops, decoder_ops_t;






#endif

