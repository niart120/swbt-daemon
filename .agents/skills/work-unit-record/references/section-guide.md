# Work Unit Record Section Guide

この reference は、work unit record の各セクションに何を書くかを定める。新規 record を作る場合、既存 record を大きく更新する場合、または記載粒度に迷う場合に読む。

## 記載粒度

work unit record は、作業中の判断と完了判定を後から追える粒度で書く。実装の実況、コマンドログ全文、調査メモの全文は置かない。必要な根拠は、spec、docs、source path、commit、実機ログへ辿れる形で要約する。

未確定の観測は `spec/dev-journal.md` に置く。複数の work unit が従う安定判断は spec に置く。work unit record には、この work unit の source、use case、範囲、検証、未完了の扱いを残す。

## セクション別ガイド

`概要` は、work unit の目的と完了後に何が変わるかを二、三段落で書く。背景の詳細や設計史は書かない。

`起点 / ユースケース` は、source と use case を分ける。source にはユーザ要求、roadmap TODO、journal entry、deferred item、bug、実機観測、根拠監査 finding、既存 spec の未解決事項を置く。use case には actor または境界、入力または状態、期待する観測結果、制約、対象外を置く。

`対象範囲` は、この work unit の完了条件に含める作業を列挙する。ここに書いたものは、検証またはチェックリストで完了状態を確認できる必要がある。

`対象外` は、開始時点でこの work unit に含めない境界を書く。対象外は失敗や未完了ではない。後から扱う可能性が高い場合でも、現時点で完了条件に含めないならここに置く。

`関連 spec / docs` は、この work unit が従う spec、更新する spec、関連 docs、根拠ログを置く。単なる参考ではなく、作業判断に影響するものを優先する。

`根拠監査` は、source-audit が必要かどうかと、その理由を書く。Switch HID、BTstack、report timing、WinUSB/libusb の事実に触れない場合だけ `not applicable` と書ける。

`設計メモ` は、この work unit 内で採用した設計判断を短く書く。複数 work unit に効く判断になったら spec に移し、この欄にはリンクだけを残す。

`対象ファイル` は、編集対象または確認対象の path を置く。全ファイルの変更ログではなく、review と handoff に必要な単位でまとめる。

`TDD Test List` は、use case から生成した観測可能な item を置く。実装都合、file list、roadmap TODO だけを item にしない。`deferred` item は、`先送り事項` の対応項目へ辿れるようにする。

`検証` は、実行したコマンド、結果、未実行理由を記録する。失敗した検証を省略しない。コマンドログ全文が長い場合は、失敗点または pass/fail が分かる要約にする。

`実機実行条件` は、実機承認の要否、adapter assumptions、environment variables、log target、cleanup requirements を書く。実機不要なら、Bluetooth adapter、Switch pairing、HID advertising、report loop に触れないことを理由として書く。

`先送り事項` は、作業中に見つかったがこの work unit では完了しない follow-up を置く。各 item には、観測、先送り理由、後続 source として使うための次の置き場を書く。何もなければ `none` と書く。

`チェックリスト` は、完了判定に必要な状態だけを置く。作業手順の細かい履歴や、検証欄と重複するコマンド列挙は置かない。
