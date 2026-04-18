(module
  (memory (export "memory") 1)
  
  ;; seraph_alloc: returns pointer to a static 100-byte buffer at address 1000
  (func (export "seraph_alloc") (param i32) (result i32)
    i32.const 1000
  )
  
  ;; seraph_free: no-op for dummy
  (func (export "seraph_free") (param i32 i32)
    nop
  )

  ;; echo_print: packs pointer and len into i64
  ;; ptr is top 32 bits, len is bottom 32.
  ;; Let's say it returns the exact same string passed in (echo).
  (func (export "print") (param $ptr i32) (param $len i32) (result i64)
    (i64.or
      (i64.shl (i64.extend_i32_u (local.get $ptr)) (i64.const 32))
      (i64.extend_i32_u (local.get $len))
    )
  )
)
