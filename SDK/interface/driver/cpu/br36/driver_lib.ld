SECTIONS
{
    .data ALIGN(4):SUBALIGN(4)
    {
        driver_data_start = .;


        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)
        *(.uart_data)

        driver_data_end = .;
        . = ALIGN(32);
    } > ram0

    .bss ALIGN(4):SUBALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)

        *(.uart_bss)

        *(.sd_var)
        	
		. = ALIGN(4);
        driver_bss_end = .;
    } > ram0

    .text  ALIGN(4):SUBALIGN(4)
    {
        driver_code_start = .;

        *(.fm_code)

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

        *(.debug_code)
        *(.debug_const)


        *(.uart_code)
        *(.uart_const)

        driver_code_end = .;
		. = ALIGN(4);
    } > code0


    .data_code : ALIGN(4)
    {
        driver_data_code_start = .;
        
		driver_data_code_end = .;
		. = ALIGN(4);
    } > ram0

    /*代码统计 Code & RAM */
    DRIVER_RAM_TOTAL   = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL  = (driver_code_end - driver_code_start);
    DRIVER_DATA_CODE_TOTAL  = (driver_data_code_end - driver_data_code_start);
}
