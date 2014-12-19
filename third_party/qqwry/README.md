 纯真IP数据库 解析 库使用说明

首先实例化 QQWry::ipdb 类
构造函数就是 QQwry.Dat 文件名

也可以自己读入QQwry.Dat 文件,然后将文件在内存中的起始位置和大小传递进来。

就这么2个重载的构造函数，呵呵

如果文件打开失败，将抛出异常,异常类型为 std::runtime_error


要查询ip所对应的地址
调用 GetIPLocation

传递 in_addr 结构
作为参数，

结果返回 IPLocation 。
内部使用了折半查找法，所以查找很迅速 (最多20次比较)，不用担心性能问题


要查询 地址所对应的ip范围
调用GetIPs
