# BTstack upstream

`swbt-daemon` は BTstack をサードパーティ依存として利用する。

配置先は次の通り。

```text
vendor/btstack
```

## 現在の基準

このプロジェクトは現在、Git submodule として BTstack `v1.8.2` に固定している。

```text
upstream repository: https://github.com/bluekitchen/btstack
submodule path: vendor/btstack
base tag: v1.8.2
base commit: 075a0780f0fad7ff67d58ac19f46e8953656a752
```

submodule を更新した場合は、正確な submodule commit をここに記録する。

## 更新方針

- 可能な限り Switch 固有の挙動は BTstack の外に置く。
- integration code は `swbt/btstack_bridge/` を優先する。
- BTstack HID validation を全体で無効化しない。
- BTstack patch または fork が必要になった場合は、この file に記録する。
