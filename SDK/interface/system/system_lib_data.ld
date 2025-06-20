
		. = ALIGN(4);
		_static_hi_timer_begin = .;
		PROVIDE(static_hi_timer_begin = .);
		KEEP(*(.hi_timer))
		_static_hi_timer_end = .;
		PROVIDE(static_hi_timer_end = .);

		. = ALIGN(4);
		_sys_cpu_timer_begin = .;
		PROVIDE(sys_cpu_timer_begin = .);
		KEEP(*(.sys_cpu_timer))
		_sys_cpu_timer_end = .;
		PROVIDE(sys_cpu_timer_end = .);
		. = ALIGN(4);



