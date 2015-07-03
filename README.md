# check_realtime

#配置说明
配置文件/usr/local/lib/libmoncfg.a示例内容如下：

新文件创建时，告警的动作；注释掉该配置表明不进行关心新文件创建。
file_created_alert = [];
文件被删除时，告警的动作
file_deleted_alert=[];
文件被修改时，采用记录日志和发送邮件的方式进行告警。
file_changed_alert = ["mail", "log"];
告警的间隔时间为30分。对同一文件的频繁修改而产生的告警，进行频次控制
alert_interval = 30;
启动时每读10个文件，sleep一次
sleep_count = 10;
每次sleep时间为1秒
sleep_interval = 1;
mail = {
     邮件服务器的地址
     server = "1.1.1.1";
     发件人的邮件
     from = "abc@xyz.com";
     接受告警的接件人的地址
     to = ["bcd@xyz.com"];
     发件人的邮件登陆用户名和密码（以Base64形式编码）
     username = "eXV4aWFvZmVuZzM=";
     password = "eXV4aWF"; 
};
checks = (
  { 
    监控的目录，不监控子目录中的文件。
path = ["/home/dir", "/home/code"];
仅对文件修改的进行日志告警
file_changed_alert = ["log"];
  },
  {
   监控的目录
   path = ["/home/ork/ "];
   仅对以.c和.cpp结尾的文件进行监控
   grep = ["\.c$"，"\.cpp$"];
   不监测名字为t.c和test.cpp的文件；不监测/home/ork/tmp目录
   filter = ["/t.c$", "/test.cpp$", “/home/ork/tmp];
   递归监控目录下的所有文件
    recursive = 1;
  }
);
#使用说明
fileuploader 的启动、停止，要以root用户来执行。
FIM使用方法
1.	Startup and Shutdown
a)	 启动FIM服务：
首次运行：
/usr/local/bin/zzhelper --start
后续运行，启动时不扫描文件：
/usr/localbin/zzhelper --start --skip
b)	停止FIM服务
/usr/local/bin/zzhelper --stop
2.	日志查看
FIM日志目录是/var/log/zzhelper.log。
