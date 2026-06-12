@echo off
echo 正在清理 Keil MDK 工程编译生成的多余文件...

:: 删除常见的中间文件扩展名
del /s /q *.o     2>nul
del /s /q *.d     2>nul
del /s /q *.crf   2>nul
del /s /q *.dep   2>nul
del /s /q *.axf   2>nul
del /s /q *.htm   2>nul
del /s /q *.lnp   2>nul
del /s /q *.sct   2>nul
del /s /q *.map   2>nul
del /s /q *.lst   2>nul
del /s /q *.i     2>nul
del /s /q *.scvd  2>nul
del /s /q *.iex   2>nul
del /s /q *.dbg   2>nul
del /s /q *.uvgui.*  2>nul
del /s /q *.uvopt*   2>nul
del /s /q *.uvguix.*  2>nul

del /s /q *_analysis.xlsx  2>nul
del /s /q *_sort_by_flash.csv  2>nul
del /s /q *_sort_by_ram.csv  2>nul
del /s /q *_alog.txt 2>nul

del /s /q JLinkLog.txt 2>nul
del /s /q JLinkSettings.ini 2>nul

:: 删除常见的输出文件夹（Objects, Listings 等）
if exist Objects   rd /s /q Objects   2>nul
if exist Listings  rd /s /q Listings  2>nul
if exist Debug     rd /s /q Debug     2>nul
if exist Release   rd /s /q Release   2>nul
if exist RTE   rd /s /q RTE   2>nul
if exist obj   rd /s /q obj   2>nul
if exist DebugConfig   rd /s /q DebugConfig   2>nul

:: 可选：删除 .hex 和 .bin 文件（如希望保留最终输出，可注释掉下面两行）
:: del /s /q *.hex   2>nul
:: del /s /q *.bin   2>nul

echo 清理完成！
pause