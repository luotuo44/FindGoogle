# FindGoogle

这个程序是用来自动查找可用的谷歌IP。
其原理是：通过TCP连接DNS服务器，然后向DNS查询谷歌的IP。接着，程序将自动尝试连接得到的谷歌IP。
当然本程序并不仅限于查询谷歌的IP，还可以查询其他域名的IP，比如facebook。

用户通过dns_query.txt文件告诉程序需要查找哪个域名的IP。由于本程序是通过DNS得到域名的IP，所以
用户还需要告诉程序DNS服务器的IP。DNS服务器IP的格式为“dns_server::208.76.50.50;216.146.35.35"(不包括双引号)

一般来说，我们只需要HTTP的谷歌即可，但有时我们还需要HTTPS的谷歌。所以用户还需要告诉程序要查询域名的
端口号。默认是443端口。域名的填写格式为"domain:www.google.com.hk:443;www.google.com"

更多的填写格式可以参考dns_query.txt文件


由于方校长对谷歌IP的443端口封杀比较严重，如果没有特殊的需求，还是建议使用"www.google.com:80"


