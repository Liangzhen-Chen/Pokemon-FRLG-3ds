# Pokémon FireRed / LeafGreen 3DS

面向 Nintendo 3DS 的《宝可梦 火红／叶绿》原生移植项目。

当前可构建版本仅提供双屏图形与输入演示，尚不能游玩完整游戏，也不支持游戏存档、中文或 Pokémon HOME 传输。程序不包含商业 ROM 或原版游戏素材。

## 构建与运行

需要 Git、Make 和 devkitPro 的 `3ds-dev` 工具链，当前目标使用 devkitARM 和 libctru。系统工具链需自行安装，项目脚本不会自动执行系统安装。

在仓库根目录运行：

```sh
make build-3ds
```

构建输出为 `platform/3ds/frlg-3ds-p2c3.3dsx` 和对应的 `.smdh`。可使用兼容的 3DS homebrew 启动器或 Azahar 打开 `.3dsx`；当前没有真机可用性保证。

演示中，方向键滚动上屏背景，A 键切换图层优先级，下屏显示按键和触控状态，同时按 X + Y 退出。更多操作说明见 [3DS 程序说明](platform/3ds/README.md)。

## 依赖与素材

第三方源码通过 Git 子模块固定，来源见 [.gitmodules](.gitmodules)，版本见 [源码依赖清单](config/source-dependencies.tsv)。需要恢复子模块时运行 `git submodule update --init`，第三方代码保留各自的许可与版权声明。

仓库不提供商业 ROM、个人存档或从 ROM 提取的资源。当前演示使用项目自有图形，无需提供 ROM。
