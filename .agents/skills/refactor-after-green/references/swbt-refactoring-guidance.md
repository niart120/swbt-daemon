# swbt Refactoring Guidance

この reference は、`refactor-after-green` の判断材料を補う。TDD の green 後に読む。作業手順の正本は `../SKILL.md` とする。

## 方針

refactor は、同じ観測結果を保ったまま変更容易性を上げる作業である。swbt では次の観測結果を変えたら behavior change と扱う。

- public C ABI の戻り値、引数解釈、エラー値。
- IPC の JSON Lines protocol、owner state、heartbeat、disconnect 時の neutral state。
- HID report bytes、report timing、subcommand response、SPI response、rumble parsing。
- BTstack bridge の callback 順序、source selection、adapter/backend 境界。
- ログ、終了状態、ユーザが確認する diagnostics。

formatter、linter、include 並べ替えだけでは refactor-done にしない。構造変更が不要なら `refactor-skipped` と記録する。

## 許容する refactor

- byte packing の重複を private helper へ寄せる。ただし期待 byte 列を変えない。
- subcommand response builder の共通 field initialization を関数化する。
- parser test の fixture setup を helper 化し、assertion の意図を読みやすくする。
- IPC parser と state mutation の混在を、小さな private 関数境界で分ける。
- 曖昧な名前を、use case と観測結果に対応する名前へ変える。
- 同じ条件分岐が近接している場合に、早期 return や小さな helper で読み順を単純にする。

## 避ける refactor

- 未検証の Switch protocol byte、report period、BTstack callback order を整理名目で変える。
- public header、IPC field、daemon lifecycle の外部契約を「ついで」に変更する。
- 今の TDD item と無関係な大きい rename、directory move、責務移動を入れる。
- まだ重複が 1 箇所しかない処理を将来予測だけで抽象化する。
- test を private helper 名、内部配列順、偶然の formatting へ依存させる。
- C code に触れていない docs / skill item で、CMake / CTest を実行したことだけを refactor の根拠にする。

## 判断の流れ

1. green baseline の command を特定する。
2. 変更対象が structure change かを確認する。迷う場合は `tidy-first` を読む。
3. 今の item に効く構造変更を 1 つ選ぶ。説明できない変更は入れない。
4. 差分が大きくなるなら `deferred` にし、後続 source として work unit record に残す。
5. 同じ command を再実行する。対象が広い場合だけ追加検証を足す。
6. `refactor-done`、`refactor-skipped`、`deferred` のいずれかを記録する。

## 例

`refactor-done`:

- report packing test を green にした後、同じ bit mask 初期化が 3 箇所にあるため private helper へ抽出した。期待 byte 列は同じで、対象 CTest を再実行した。
- JSON Lines parser の error case を追加した後、test fixture の入力構築を helper 化した。assertion は parser の観測結果のままにした。

`refactor-skipped`:

- green 後に重複を確認したが、重複は 1 箇所だけで抽象化名が protocol 未検証値を隠すため、構造変更しなかった。
- docs / skill guidance の typo 修正だけで、構造変更の対象がなかった。

`deferred`:

- IPC parser と daemon state の責務分離が必要だが、今の parser error item より大きい。後続 work unit の source として残す。
- BTstack bridge の callback ordering を整理したいが、根拠監査と実機観測が必要なため、この cycle では扱わない。
