(module
  (memory (export "memory") 1)
  
  ;; Alloc returns an address strictly too close to the edge of the 1-page max (65536).
  ;; E.g., if we request size 10, 65530 + 10 = 65540 (OOB before writing payload).
  (func (export "seraph_alloc") (param i32) (result i32)
    i32.const 65530
  )
  
  (func (export "seraph_free") (param i32 i32)
    nop
  )

  (func (export "print") (param $ptr i32) (param $len i32) (result i64)
    (i64.or
      (i64.shl (i64.extend_i32_u (local.get $ptr)) (i64.const 32))
      (i64.extend_i32_u (local.get $len))
    )
  )
)
