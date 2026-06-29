# Taco 语言设计文档 v1

> A simple, spicy scripting language.

---

## 设计哲学

- 语法通用，不自搞一套，程序员一看就懂
- 关键字尽可能少
- 命名风格：动词+名词，短，清晰
- 足够透明，不抢镜，让代码自然地讲故事
- 住在终端里，打开就写，写完就跑

---

## 基本语法

### 变量
```taco
var x = 10;
var name = "Miguel";
var flag = true;
var nothing = nil;
```
变量默认局部作用域。

### 注释
```taco
// 这是注释
```

### 分号
每行结尾需要分号。

---

## 数据类型

### 基本类型
- `number`：整数和浮点数统一
- `string`：字符串
- `bool`：`true` / `false`
- `nil`：空值

### 数字
```taco
var x = 10;
var y = 3.14;
var big = 1_000_000;
```

### 字符串
```taco
var name = "Miguel";
var greeting = "Hola, {name}!";  // 字符串插值
```

### array
```taco
var arr = [1, 2, 3, 4, 5];
var first = arr[0];              // 从 0 开始
var nested = [[1, 2], [3, 4]];
var merged = [...arr, 6, 7];    // 展开运算符
```

### map
```taco
var person = {"name": "Miguel", "age": 12};
var name = person["name"];
var name2 = person.name;        // 点语法
var nested = {
    "users": [
        {"name": "Miguel", "age": 12}
    ]
};
```

---

## 运算符

### 算术
```taco
x + y
x - y
x * y
x / y
x % y
x ^ y    // 幂运算
```

### 比较
```taco
x == y
x != y
x > y
x < y
x >= y
x <= y
```

### 逻辑
```taco
x && y
x || y
!x
```

### 字符串
```taco
"hello" + " world"    // 拼接
"Hola, {name}!"       // 插值
```

### 其他
```taco
x > 0 ? "positive" : "negative"   // 三元
user?.address?.city                // 可选链
value ?? "default"                 // 默认值
[...arr1, ...arr2]                 // 展开
```

---

## 控制流

### if / elseif / else
```taco
if (x > 90) {
    print("A");
} elseif (x > 80) {
    print("B");
} else {
    print("C");
}
```

### while
```taco
while (x > 0) {
    x = x - 1;
}
```

### for（C 风格）
```taco
for (var i = 0; i < 10; i++) {
    print(i);
}
```

### for（range 风格）
```taco
for i in range(0, 10) {
    print(i);
}
```

### switch
```taco
switch x {
    case 1 {
        print("one");
    }
    case 2 {
        print("two");
    }
    default {
        print("other");
    }
}
```
每个 case 默认不穿透，不需要 break。

---

## 函数

### 基本函数
```taco
func greet(name) {
    print("Hola, {name}!");
}

greet("Miguel");
```

### 多返回值
```taco
func minmax(arr) {
    return arr.getFirst(), arr.getLast();
}

var min, max = minmax([3, 1, 4, 1, 5]);
```

### 命名参数
```taco
func greet(name, greeting: "Hola") {
    print("{greeting}, {name}!");
}

greet("Miguel");
greet("Dante", greeting: "Hey");
```

### 递归
```taco
func fib(n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
```

### 闭包
```taco
var double = { x in x * 2 };
var add = { (x, y) in x + y };

// 用在 pipeline
nums.filter { x in x > 3 }.each { x in print(x) };
```

---

## OOP

### class（引用类型）
```taco
class Animal {
    var name;

    func init(name) {
        self.name = name;
    }

    func speak() {
        print("{self.name} makes a sound");
    }
}

class Dog extends Animal {
    func speak() {
        print("{self.name} barks");
    }
}

var d1 = Dog("Dante");
var d2 = d1;        // 引用，同一个对象
d2.name = "Coco";
print(d1.name);     // Coco
```

### struct（值类型）
```taco
struct Point {
    var x;
    var y;

    func distanceTo(other) {
        var dx = self.x - other.x;
        var dy = self.y - other.y;
        return (dx ^ 2 + dy ^ 2) ^ 0.5;
    }
}

var p1 = Point(1, 2);   // 自动 init
var p2 = p1;            // 拷贝
p2.x = 10;
print(p1.x);            // 1，p1 没变
```

### enum
```taco
enum Direction {
    North,
    South,
    East,
    West
}

var d = Direction.North;

switch d {
    case Direction.North { print("going north"); }
    case Direction.South { print("going south"); }
    default { print("other"); }
}

// 带关联值
enum Result {
    Ok(value),
    Err(message)
}

var r = Result.Ok(42);
var e = Result.Err("something went wrong");
```

---

## 并发

### thread
```taco
var t = thread {
    print("running in background");
};
t.join();
t.detach();
```

### channel
```taco
var ch = channel();

var producer = thread {
    for i in range(0, 5) {
        ch.send(i);
    }
    ch.close();
};

var consumer = thread {
    while (ch.isOpen()) {
        var val = ch.receive();
        print(val);
    }
};

producer.join();
consumer.join();
```

### mutex
```taco
var mu = mutex();
var count = 0;

var t = thread {
    mu.lock();
    count = count + 1;
    mu.unlock();
};

t.join();
```

---

## 模块系统

```taco
import utils;
import io;
from io import readFile;
import utils as u;

// 标准库
import taco.io;
import taco.net;
import taco.thread;
import taco.random;
from taco.random import pick, flip;
```

---

## 标准库（命名风格：动词+名词，短）

### string
```taco
str.getLines()          // 按行切分
str.getWords()          // 按空格切分
str.getChars()          // 按字符切分
str.split(",")          // 按分隔符切分
str.trimSpace()         // 去除首尾空格
str.findStr("hello")    // 查找子串
str.replaceStr(a, b)    // 替换
str.startsWith("http")
str.endsWith(".log")
str.contains("error")
str.upper()
str.lower()
str.len()
```

### array
```taco
arr.getFirst()
arr.getFirst(5)
arr.getLast()
arr.getLast(50)
arr.filter { x in x > 0 }
arr.map { x in x * 2 }
arr.each { x in print(x) }
arr.reduce { (acc, x) in acc + x }
arr.find { x in x > 10 }
arr.findFirst()
arr.findLast()
arr.sortBy { x in x }
arr.groupBy { x in x % 2 }
arr.countBy { x in x > 0 }
arr.push(value)
arr.pop()
arr.len()
arr.contains(value)
arr.sum()
arr.avg()
arr.min()
arr.max()
```

### map
```taco
map.getKeys()
map.getValues()
map.has("key")
map.remove("key")
map.len()
```

### 文件系统
```taco
cat("file.txt")           // 读文件内容
echo("content", "file")   // 写文件
ls(".")                   // 列目录
mkdir("path")             // 创建目录
rm("file.txt")            // 删除文件
mv("src", "dst")          // 移动/重命名
cp("src", "dst")          // 复制
pwd()                     // 当前目录
cd("path")                // 切换目录
exists("path")            // 是否存在
```

### 网络
```taco
fetchUrl("https://api.example.com")
postData("https://api.example.com", data)
```

### I/O
```taco
print(value)
input("prompt")
```

### 类型转换
```taco
number(value)
string(value)
bool(value)
type(value)
```

### 系统
```taco
exec("git status")
env("HOME")
```

### random
```taco
random.pick([1, 2, 3])
random.flip()
random.int(0, 100)
random.float(0.0, 1.0)
```

---

## 错误处理

出错直接崩溃，打印清晰的错误信息：

```
🌮 line 5: 'naem' is not defined. Did you mean 'name'?
    var x = naem + 1;
            ^^^^
```

---

## REPL

```
🌮 Taco 0.1.0
   It works on my machine.
> var name = "Miguel";
> print("Hola, {name}!");
Hola, Miguel!
> 1 + 1
2
> exit
🌮 Fine.
```

---

## 文件

- 文件后缀：`.taco`
- 运行方式：`taco script.taco`
- REPL：`taco`


---

## 彩蛋：Magic 8 Ball 🌮

🌮 是 Taco 的隐藏彩蛋，有两种用法：

### 独立语句：问问题
```taco
🌮 "Will my code compile?";
// 🎱 Don't count on it.

🌮 "Should I refactor this?";
// 🎱 Signs point to yes.

🌮 "Is this a bug or a feature?";
// 🎱 Very doubtful.
```

### 作为值：返回随机答案字符串
```taco
var answer = 🌮;
print(answer);
// 🎱 It is certain.

print(🌮);
// 🎱 Without a doubt.

// 字符串插值里用
print("The magic ball says: {🌮}");
// The magic ball says: 🎱 Ask again later.

// 放在 pipeline 里
var scores = [88, 45, 92];
scores
    .filter { s in s > 60 }
    .each { s in print("{s}: {🌮}") };
// 88: 🎱 It is certain.
// 92: 🎱 Ask again later.
```

### 20 个经典答案

**肯定**
- It is certain.
- Without a doubt.
- Yes, definitely.
- You may rely on it.
- Most likely.
- Outlook good.
- Yes.
- Signs point to yes.
- As I see it, yes.
- It is decidedly so.

**中立**
- Reply hazy, try again.
- Ask again later.
- Cannot predict now.
- Concentrate and ask again.
- Better not tell you now.

**否定**
- Don't count on it.
- My reply is no.
- Very doubtful.
- Outlook not so good.
- My sources say no.

### 在 REPL 里
```
🌮 Taco 0.1.0
   It works on my machine.
> 🌮 "Will I finish this project?";
🎱 Outlook not so good.
> print(🌮);
🎱 Without a doubt.
>
```


---

## 彩蛋：🌮 的五种性格

同一个符号，数量不同，性格完全不同。

### 一个 🌮：Magic 8 Ball
神秘认真，随机返回一个答案。

```taco
🌮 "Will my code compile?";
// 🎱 It is certain.

🌮 "Should I refactor this?";
// 🎱 Don't count on it.

var answer = 🌮;
print("The magic ball says: {🌮}");

// 放在 pipeline 里
scores
    .filter { s in s > 60 }
    .each { s in print("{s}: {🌮}") };
// 88: 🎱 It is certain.
// 92: 🎱 Ask again later.
```

20 个经典答案随机出现（见上方列表）。

---

### 两个 🌮🌮：魔法海螺
废话连篇，随机返回一个海螺回答。

```taco
🌮🌮 "Will my code compile?";
// No, no, no, no, no.

🌮🌮 "Should I refactor this?";
// Maybe someday.

var b = 🌮🌮;
// I don't think so.
```

经典回答：
- No.
- I don't think so.
- No, no, no, no, no.
- Maybe someday.
- Yes.
- Try asking again.
- Neither.
- Mmm, I don't think so.
- No, definitely not.

---

### 三个 🌮🌮🌮：随机笑话
完全不搭，随机输出一个程序员笑话。

```taco
🌮🌮🌮 "Will my code compile?";
// Why do programmers prefer dark mode?
// Because light attracts bugs.

🌮🌮🌮;
// A SQL query walks into a bar, walks up to two tables and asks...
// "Can I join you?"
```

---

### 四个 🌮🌮🌮🌮：Unix 时间戳
冷静，精确，完全不搭。输出从 1970 年到现在的秒数。

```taco
🌮🌮🌮🌮;
// 1719556800
```

没有任何解释。懂的人秒懂，不懂的人去查，查完更好笑。

---

### 五个及以上 🌮🌮🌮🌮🌮+：收尾
打破第四堵墙，语言本身跟你说话。

```taco
🌮🌮🌮🌮🌮;
// That's enough tacos for today.
```

不管打多少个，五个以上永远输出这句话。

---

### 发现顺序

用户发现这五个彩蛋的过程本身就是乐趣：
1. 第一次用 🌮，发现 Magic 8 Ball
2. 手滑打了两个，发现魔法海螺，愣一下
3. 好奇打三个，出来一个笑话，笑出来
4. 继续试四个，得到一串数字，去查是什么
5. 打五个，语言叫你停了


---

## 彩蛋续：100个 🌮 和 50个 🌮 的完整设计

### 99个 🌮：👄 吃 taco 动画

👄 从左往右移动，每吃一个 🌮 停一下，最后变成 😊：

```
👄🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮
 👄🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮
  👄🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮🌮
...
                                                  👄🌮
                                                   😊
¡El Taco es muy delicious! Although I have never eaten one.
```

仅在命令行模式下触发。

---

### 42个 🌮：La Llorona 歌词

逐句显示 La Llorona 传统民谣，每句西班牙语后停一秒，再显示英文和中文翻译。仅在命令行模式下触发。

（42——宇宙的答案，是一首墨西哥民谣。）

```
Ay, de mí, llorona, Llorona de azul celeste
Ay, de mí, llorona, Llorona de azul celeste
  Woe is me, Weeping Lady, dressed in light blue
  唉，哭泣的女人，身着天蓝色

Y aunque la vida me cueste, llorona, No dejaré de quererte
No dejaré de quererte
  Even if it costs me my life, I will not stop loving you
  即使付出生命，我也不会停止爱你

Me subí al pino más alto, llorona, A ver si te divisaba
Me subí al pino más alto, llorona, A ver si te divisaba
  I climbed the highest pine tree, just to catch a glimpse of you
  我爬上最高的松树，只为看你一眼

Como el pino era tierno, llorona, Al verme llorar, lloraba
Como el pino era tierno, llorona, Al verme llorar, lloraba
  The pine was young and tender, and seeing me cry, it cried too
  那松树还很嫩，见我哭泣，它也跟着哭

La pena y la que no es pena, llorona, Todo es pena para mí
La pena y la que no es pena, llorona, Todo es pena para mí
  Sorrow and what is not sorrow — everything is sorrow to me
  悲与不悲，对我来说都是悲

Ayer lloraba por verte, llorona, Hoy lloro porque te vi
Ayer lloraba por verte, llorona, Hoy lloro porque te vi
  Yesterday I wept to see you, today I weep because I did
  昨天哭着想见你，今天见了还是哭

Ay, de mí, llorona, llorona, Llorona de azul celeste
Ay, de mí, llorona, llorona, Llorona de azul celeste
  Woe is me, Weeping Lady, Weeping Lady dressed in light blue
  唉，哭泣的女人，身着天蓝色的女人

Y aunque la vida me cueste, llorona, No dejaré de quererte
Y aunque la vida me cueste, llorona, No dejaré de quererte
No dejaré de quererte, No dejaré de quererte
  Even if it costs me my life, I will not stop loving you
  即使付出生命，我也不会停止爱你
  我不会停止爱你，我不会停止爱你
```

歌词来源：传统墨西哥民谣，公共领域。


---

### 100个 🌮：La Llorona 的故事

逐段显示 La Llorona 的民间传说，三语对照，每段停两秒再继续。仅在命令行模式下触发。

```
~ La Llorona ~

ES:
Hace mucho tiempo, en un pequeño pueblo de México,
vivía una mujer hermosa llamada María.
Era tan bella que todos los hombres del pueblo
la miraban al pasar.

EN:
Long ago, in a small town in Mexico,
there lived a beautiful woman named María.
She was so lovely that all the men in the village
would watch her as she passed.

中：
很久以前，在墨西哥的一个小镇上，
住着一位名叫玛利亚的美丽女子。
她是如此美丽，村里所有的男人
都会在她经过时驻足凝望。

---

ES:
María se enamoró de un hombre rico y apuesto.
Tuvieron hijos juntos, pero el hombre
la abandonó por otra mujer de mayor riqueza.
El corazón de María se llenó de oscuridad.

EN:
María fell in love with a rich and handsome man.
They had children together, but the man
left her for a wealthier woman.
María's heart filled with darkness.

中：
玛利亚爱上了一个富有英俊的男人。
他们生儿育女，但那个男人
为了一个更富有的女人抛弃了她。
玛利亚的心充满了黑暗。

---

ES:
En su dolor y locura,
llevó a sus hijos al río
y los dejó que las aguas se los llevaran.
Al instante, comprendió lo que había hecho.

EN:
In her grief and madness,
she took her children to the river
and let the waters take them away.
In an instant, she understood what she had done.

中：
在悲痛与疯狂中，
她带着孩子们来到河边，
让河水将他们带走。
就在那一瞬间，她明白了自己做了什么。

---

ES:
María lloró y lloró,
buscando a sus hijos por las orillas del río.
Pero era demasiado tarde.
Murió de pena a la orilla del agua.

EN:
María wept and wept,
searching for her children along the riverbanks.
But it was too late.
She died of grief at the water's edge.

中：
玛利亚哭啊哭啊，
沿着河岸寻找她的孩子们。
但已经太迟了。
她在水边因悲痛而死去。

---

ES:
Desde entonces, su espíritu vaga por los ríos
y los lagos de México, llorando eternamente:
"¡Ay, mis hijos!"
Los niños que la oyen... desaparecen.

EN:
Since then, her spirit wanders the rivers
and lakes of Mexico, weeping forever:
"Oh, my children!"
The children who hear her... disappear.

中：
从那以后，她的灵魂在墨西哥的
河流和湖泊间游荡，永远哭泣着：
"唉，我的孩子们！"
听到她哭声的孩子们……就此消失。

---

ES:
Si escuchas llorar en la noche,
no te asomes a la ventana.
Es La Llorona.
Y te está buscando a ti.

EN:
If you hear weeping in the night,
do not look out the window.
It is La Llorona.
And she is looking for you.

中：
如果你在夜里听到哭声，
不要向窗外望去。
那是哭泣的女人。
她在找的，是你。

~ Fin ~
```

