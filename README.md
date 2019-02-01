# LKL BBR Portforwarder

使用LKL BBR Portforwarder可以不受内核限制，在用户空间实现启用BBR的端口转发。

## 环境要求

- 目前测试过Ubuntu 16.04和CentOS 7.0环境；
- 必须用root权限运行；
- 必须有TUN/TAP支持；
- IP转发的环境要求见下面的“关于IP转发”小节。

## 在Ubuntu安装（16.04）

### 安装开发环境
```
apt install libfuse-dev libarchive-dev xfsprogs btrfs-tools gcc bison flex bc
```

### 编译安装liblkl
```
wget https://github.com/lkl/linux/archive/master.tar.gz -O linux-master.tar.gz
tar xvzf linux-master.tar.gz
cd linux-master/tools/lkl
make
make install
```

### 编译安装lkl_bbr_portforwarder
```
wget https://github.com/k9yyy/lkl_bbr_portforwarder/archive/master.tar.gz -O lkl_bbr_portforwarder-master.tar.gz 
tar xvzf lkl_bbr_portforwarder-master.tar.gz 
cd lkl_bbr_portforwarder-master
./configure
make
make install
```

## 使用方法
```
lkl_bbr_portforwarder -c <配置文件路径>
```
配置文件样本见`example.cfg`。

如用于给Shadowsocks加速，将配置文件里的`listen_ip6_address`和`listen_ip_address`的值改为要监听的IP地址，`listen_port`的值改为要监听的端口号，`target_port`的值改为本地Shadowsocks监听的端口号。

## 配置项
配置文件中主要有LKL网络设置、NAT设置、TCP转发设置三块。

### LKL网络设置
如果没有与现有网络配置冲突，本块内容可以直接使用`example.cfg`中写好的。
* `lkl_mac_address`、`tap_mac_address`：设定LKL和TAP网卡的MAC地址。
* `lkl_ip_address`、`tap_ip_address`：设定LKL和TAP网卡的IPv4地址。
* `lkl_ip_subnet_prefix_len`：设定LKL的IPv4网络子网前缀长度。必须能使上面的LKL和TAP网卡的IPv4地址在同一个子网内。
* `lkl_ip6_address`、`tap_ip6_address`：设定LKL和TAP网卡的IPv6地址。
* `lkl_ip_subnet_prefix_len`：设定LKL的IPv6网络子网前缀长度。必须能使上面的LKL和TAP网卡的IPv6地址在同一个子网内。
* `ip6_support`：是否启用IPv6支持。（`true`或`false`）

### NAT配置
LKL BBR Portforwarder使用NAT来使外网来的TCP连接可以进入LKL的网卡。

* `set_iptables_nat`、`set_ip6tables_nat`：是否自动执行`iptables`、`ip6tables`命令来用iptables进行NAT。（`true`或`false`）
* `enable_ip6_forwarder`、`enable_ip6_forwarder`：是否使用LKL BBR Portforwarder内置的IPv4、IPv6转发器。（`true`或`false`）

关于上面两项怎么选择，具体参见下面的”关于IP转发“小节。

* `listen_ip_address`、`listen_ip6_address`：要监听的出口IPv4和IPv6地址。因为实际上是用于指定NAT的中的用到的目标IP和来源IP参数，不可以是`0.0.0.0`或`::`。使用自动设定iptables NAT或者内置转发器都需要设定此项。
* `ip_forwarder_listen_interface`、`ip6_forwarder_listen_interface`：要监听的出口网卡接口名（IPv4、IPv6）。使用内置转发器需要设定此项。
* `set_iptables_drop_rst`、`set_ip6tables_drop_rst`：是否自动执行`iptables`、`ip6tables`命令来过滤内核发出的RST包。使用内置转发器需要设定此项，否则需要手动添加规则。（`true`或`false`）

### 端口转发配置
* `target_ip_address`：目标IP地址。目前仅支持IPv4。
* `target_port`：目标端口号。
* `listen_port`：监听端口号。此项也用于NAT中的参数。

### 其他
* `lkl_mem`：分配给LKL的内存大小，单位为M。设置得太小可能会影响性能。

## 关于IP转发
根据主机是否载入了`iptable_nat`内核模块，`set_iptables_nat`和`enable_ip_forwarder`中可以选择一个设为`true`。如果没有载入`iptable_nat`内核模块，iptables的NAT表无法使用，只能使用内置的IP转发器。如果启用IPv6支持（`ip6_support`设为`true`），`set_ip6tables_nat`和`enable_ip6_forwarder`中也可以选择一个设为`true`。如果内核、iptables版本支持，并且有载入`ip6table_nat`内核模块，则可以使用`set_ip6tables_nat`。OpenVZ 6的内核不支持用ip6tables做NAT，必须使用`enable_ip6_forwarder`。注意内置的IP转发器仅支持OpenVZ的网络环境（网关必须是Cooked-mode）。

## 内核IP转发配置
要使LKL可以访问外网，必须设置内核属性，启用IP转发。因为目前TCP转发目标只能是IPv4，如果不使用`set_ip6tables_nat`的话只需要启用IPv4的即可。内置转发器不需要用到内核的IP转发。

编辑`/etc/sysctl.conf`，找到如下内容并取消注释（如果没有则插入）：

### IPv4
```
net.ipv4.ip_forward=1
```
### IPv6
```
net.ipv6.conf.all.forwarding=1
```
然后执行`sysctl -p`重新载入配置。

如果是临时使用，也可以用`sysctl -w net.ipv4.ip_forward=1`和`sysctl -w net.ipv6.conf.all.forwarding=1`做一次性的设置。重启后会失效。

## 关于iptables配置
根据配置文件的设置，LKL BBR Portforwarder会自动添加需要的iptables规则，并在退出时自动删除，正常情况下无需额外配置。

## Checksum offload可能对内置转发器产生影响
在使用内置转发器时，如果出现大量checksum有误导致数据包被丢弃的情况，请尝试关闭网卡的checksum offload。网卡的checksum offload可能会导致软件收到的数据包中的checksum和实际收到的不同。

用`ethtool`关闭checksum offload的方法：
```
ethtool --offload <网卡> rx off tx off
```

## 注意事项
* 使用`set_ip(6)tables_nat`或`enable_ip(6)_forwarder`均为默认允许所有来源，并且不经过iptables的INPUT链。
* 这是一个玩具项目，不要在任何生产环境使用。