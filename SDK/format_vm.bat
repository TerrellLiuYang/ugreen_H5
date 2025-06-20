SET PROJ_DOWNLOAD_PATH=%1
if %2_A==all_A (
    SET FORMAT_ALL_ENABLE=1
) else (
    SET FORMAT_VM_ENABLE=1
)
cpu\br36\tools\download.bat
