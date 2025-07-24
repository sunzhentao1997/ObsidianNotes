###### 1、新建工程使用Commercial Part Number搜索芯片型号，输入一个字母出现两个
![](pic/STM32Cube%20搜索功能1.png)
解决方案：中文输入法导致的，搜索时写换到英文输入法即可。
###### 2、STM32Cube IDE/MX 使用Series过滤芯片，芯片列表是空的
![|825](pic/STM32Cube%20搜索功能2.png)
解决方案：①  Window --> Preferences --> STM32Cube --> Firmware Updater --> 
			Target Selector Device Database Auto-Refresh --> 选择“No Auto-Refresh at Application start”
		  ② 搜索 ".stmcufinder" 文件夹，删除文件夹下的所有文件
