# Architecture Specs

daemon 構成、責務境界、module boundary、BTstack bridge など、現在有効な構造の spec を置く。

このディレクトリの文書は、実装や work unit record が従う規範として扱う。
upstream 調査や根拠の要約だけを置く場合は `spec/references/` を使う。

## 現在の spec

- [Daemon Runtime Boundaries](daemon-runtime-boundaries.md): daemon runtime、IPC、Switch protocol core、BTstack bridge、fake backend、実機実行条件の責務境界。
