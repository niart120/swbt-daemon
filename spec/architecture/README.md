# Architecture Specs

daemon 構成、責務境界、module boundary、BTstack bridge など、現在有効な構造の spec を置く。

このディレクトリの文書は、実装や work unit record が従う規範として扱う。
upstream 調査や根拠の要約だけを置く場合は `spec/references/` を使う。

## 現在の spec

- [Daemon Architecture Cutover](daemon-architecture-cutover.md): daemon logical state、IPC、BTstack adapter、host composition、platform shutdown の current architecture policy。
- [Bond Cache Persistence](bond-cache-persistence.md): BTstack Classic link key DB / TLV bond cache の内部運用境界、cleanup、外部契約化条件。
