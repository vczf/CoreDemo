# CoreDemo
本项目基于hustoj进行二次开发。

_ _ _ _ _ _

# 手动安装
## 安装数据库
在终端内
```
sudo apt-get install mysql-server mysql-client
sudo apt-get install libmysqlclient15-dev
```

_ _ _ _ _ _

## 创建数据库
mysql 
```
create database zhbitoj;
use zhbitoj;
source db.sql
```

_ _ _ _ _ _

### 自行创建管理员
*请先在web页面注册一个普通用户*
```
update user_info set role=3 where user_name='admin';
```


_ _ _ _ _ _
## 添加判题用户

```
useradd --uid 2333 judge
```

_ _ _ _ _ _

## 创建对应目录
```
mkdir /home/judge/
mkdir /home/judge/etc/
mkdir /home/judge/data/
mkdir /home/judge/run0/
mkdir /home/judge/log/
mkdir /home/judge/pipe/
```
目前默认只有一个判题机,所以只需要创建一个run0,如果要多个判题机,比如说需要2个,则创建/home/judge/run0 和/home/judge/run1 目录

## 判题配置文件
在配置判题配置文件之前,请先
```
cp judge.conf /home/judge/etc/judge.conf
vim /home/judge/etc/judge.conf
```
### 参数解释

```
DB_ADDRESS=127.0.0.1    #数据库ip
DB_NAME=zhbitoj         #数据库名
DB_USER_NAME=admin      #数据库用户名
DB_PASSWORD=0000        #数据库密码
OJ_SLEEP_TIME=1         #如果没有需要judge的记录时等待多久单位秒
OJ_CLIENT_NUM=1         #判题机数量 默认1 因为还有bug,所以只能1
```
编辑完毕后将注释信息删除









