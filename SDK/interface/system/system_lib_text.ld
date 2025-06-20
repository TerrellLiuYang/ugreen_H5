
		. = ALIGN(4);
        system_text_start = .;

		 _device_node_begin = .;
   		PROVIDE(device_node_begin = .);
   		KEEP(*(.device))
   		_device_node_end = .;
   		PROVIDE(device_node_end = .);

		config_target_begin = .;
   		PROVIDE(config_target_begin = .);
   		KEEP(*(.config_target))
		config_target_end = .;
   		PROVIDE(config_target_end = .);

	    system_code_begin = .;
	    KEEP(*(.system.*.text))
	    system_code_end = .;
		. = ALIGN(4);
		system_code_size = system_code_end - system_code_begin;	

	    _lib_version_begin = .;
	    PROVIDE(lib_version_begin = .);
	    KEEP(*(.lib_version))
	    _lib_version_end = .;
	    PROVIDE(lib_version_end = .);

		_initcall_begin = .;
		PROVIDE(initcall_begin = .);
	    KEEP(*(.initcall))
		_initcall_end = .;
		PROVIDE(initcall_end = .);

		_early_initcall_begin = .;
		PROVIDE(early_initcall_begin = .);
		KEEP(*(.early.initcall))
		_early_initcall_end = .;
		PROVIDE(early_initcall_end = .);

		_late_initcall_begin = .;
		PROVIDE(late_initcall_begin = .);
		KEEP(*(.late.initcall))
		_late_initcall_end = .;
		PROVIDE(late_initcall_end = .);

		_platform_initcall_begin = .;
		PROVIDE(platform_initcall_begin = .);
		KEEP(*(.platform.initcall))
		_platform_initcall_end = .;
		PROVIDE(platform_initcall_end = .);

		_module_initcall_begin = .;
		PROVIDE(module_initcall_begin = .);
		KEEP(*(.module.initcall))
		_module_initcall_end = .;
		PROVIDE(module_initcall_end = .);

		platform_uninitcall_begin = .;
		KEEP(*(.platform.uninitcall))
		platform_uninitcall_end = .;

		_sys_event_handler_begin = .;
		PROVIDE(sys_event_handler_begin = .);
		KEEP(*(.sys_event.4.handler))
		KEEP(*(.sys_event.3.handler))
		KEEP(*(.sys_event.2.handler))
		KEEP(*(.sys_event.1.handler))
		KEEP(*(.sys_event.0.handler))
		_sys_event_handler_end = .;
		PROVIDE(sys_event_handler_end = .);

		_syscfg_ops_begin = .;
		PROVIDE(syscfg_ops_begin = .);
		KEEP(*(.syscfg.2.ops))
		KEEP(*(.syscfg.1.ops))
		KEEP(*(.syscfg.0.ops))
		_syscfg_ops_end = .;
		PROVIDE(syscfg_ops_end = .);

		. = ALIGN(4);
	    lp_target_begin = .;
	    PROVIDE(lp_target_begin = .);
	    KEEP(*(.lp_target))
	    lp_target_end = .;
	    PROVIDE(lp_target_end = .);

		. = ALIGN(4);
	    lp_request_begin = .;
	    PROVIDE(lp_request_begin = .);
	    KEEP(*(.lp_request))
	    lp_request_end = .;
	    PROVIDE(lp_request_end = .);

		. = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

		. = ALIGN(4);
		p2m_msg_handler_begin = .;
		PROVIDE(p2m_msg_handler_begin = .);
		KEEP(*(.p2m_msg_handler))
		PROVIDE(p2m_msg_handler_end = .);
		p2m_msg_handler_end = .;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        *(.math_fast_funtion_code)
		. = ALIGN(4);

        system_text_end = .;

        system_code_total_size = system_text_end - system_text_start;




