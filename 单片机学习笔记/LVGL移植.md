# LVGL下载源码
GitHub远程仓库地址:https://github.com/lvgl/lvgl.git 。
下载LVGLv8.3版本，配合NXP软件 <font color=red>GUI-Guider-1.8.1</font> 使用。
![](pic/Pasted%20image%2020250725153446.png#pic_center|825)
														LVGL源码目录
# 源代码移植
## 1、修改文件配置
① 将源码目录下的 "lv_conf_template.h"改为 “lv_conf.h”
② 打开 “lv_conf.h” 文件，将文件中的 “#if 0” 改为 “#if 1”
![](pic/Pasted%20image%2020250725154846.png)
③ 删除下图之外的所有文件及文件夹
![|695](pic/Pasted%20image%2020250725161051.png)
## 2、将文件导入到工程中
① 添加工程分组
![|605](pic/Pasted%20image%2020250725161500.png)
② 往 Middlewares/lvgl/src/core 分组中添加 core 文件夹下的全部.c 文件
![|186](pic/Pasted%20image%2020250725161856.png)
③ 往 Middlewares/lvgl/src/draw 组中添加 draw 文件夹下除 nxp_pxp、nxp_vglite、sdl 和 stm32_dma2d 文件夹之外的全部.c 文件
![|186](pic/Pasted%20image%2020250725161925.png)
④ 往 Middlewares/lvgl/src/extra 组中添加 extra 文件夹下除了 lib 文件夹之外的全部.c 文件
![|186](pic/Pasted%20image%2020250725162058.png)
⑤ 往 Middlewares/lvgl/src/font 组中添加 font 文件夹下的全部.c 文件
![186](pic/Pasted%20image%2020250725162216.png)
⑥ 往 Middlewares/lvgl/src/gpu 组中添加 draw/stm32_dma2d 和 draw/sdl文件夹下的全部.c文件
![|186](pic/Pasted%20image%2020250725162334.png)
⑦ 往 Middlewares/lvgl/src/hal 组中添加 hal 文件夹下的全部.c 文件
![|186](pic/Pasted%20image%2020250725162451.png)
⑧ 往 Middlewares/lvgl/src/misc 组中添加 misc 文件夹下的全部.c 文件
![|186](pic/Pasted%20image%2020250725162824.png)
⑨ 往 Middlewares/lvgl/src/widgets 组中添加 widgets 文件夹下的全部.c 文件
![|186](pic/Pasted%20image%2020250725162926.png)
⑩ Middlewares/lvgl/examples/porting 组添加 Middlewares/LVGL/GUI/lvgl/examples/porting目录下的 lv_port_disp_template.c 和lv_port_indev_template.c 文件
![](pic/Pasted%20image%2020250725163056.png)
# UI移植
## 1、UI制作

见 《                                                           》
## 2、代码生成

制作好UI后点击生成代码(快捷键Ctrl + G);
点击代码导出;
## 3、将代码文件导入到工程

①  将“custom” 文件夹下的 “custom.c”、“custom.h”，“generated” 文件夹下的所有.h .c文件，”generated/images“ 文件夹下的所有.h .c文件，添加到 Middlewares/LVGL/GUI_APP 组
![|186](pic/Pasted%20image%2020250725165721.png)
② 将“generated/guider_customer_fonts”、“generated/guider_customer_fonts”文件夹下所有的.h .c 文件添加到Middlewares/LVGL/GUI_APP/GUI_FONT 中。
![|186](pic/Pasted%20image%2020250725170036.png)
# 修改触摸显示驱动
详细见 “LVGL/example/porting”文件夹下的 lv_port_disp_template.c/h 和 lv_port_indev_template.c/h 。
