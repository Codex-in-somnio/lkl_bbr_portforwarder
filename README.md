# LKL BBR Portforwarder

## 在Ubuntu安装（16.04）
### 安装开发环境
```
apt install libfuse-dev libarchive-dev xfsprogs btrfs-tools gcc bison flex bc
```

### 编译安装liblkl
```
git clone https://github.com/lkl/linux.git
cd linux/tools/lkl
make
make install
```

### 编译安装lkl_bbr_portforwarder
```
cd lkl_bbr_portforwarder
tar xvzf lkl_bbr_portforwarder-0.1.tar.gz
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

如果启用IPv6支持（`ip6_support`设为`true`），`set_ip6tables_nat`和`enable_ip6_forwarder`中需选择一个设为`true`。OVZ6的内核不支持用ip6tables做NAT，需要使用`enable_ip6_forwarder`启用内置的IPv6转发；OVZ7或其他内核版本大于等于3.9的环境，则可以使用`set_ip6tables_nat`来使用ip6tables实现。内置的IPv6转发仅支持OVZ的网络环境（网关必须是Cooked-mode）。

## IP转发配置
编辑```/etc/sysctl.conf```，找到如下内容并取消注释（如果没有则插入）：

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

## 防火墙配置
LKL BBR Portforwarder会自动添加需要的iptables规则，并在退出时自动删除，正常情况下无需额外配置。
