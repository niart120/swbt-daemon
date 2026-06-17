# Work Units

このディレクトリには work unit record を置く。

work unit は作業管理上のまとまりであり、文書そのものではない。
work unit record は 1 つの work unit の範囲、関連 spec/docs、検証、根拠監査状態、実機状態、完了条件を束ねる記録である。

作業中の work unit record は `work-units/wip/` に置く。
完了した work unit record は `work-units/complete/` に移す。
