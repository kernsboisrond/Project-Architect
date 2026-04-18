(module
  (memory (export "memory") 1)
  
  (func (export "seraph_alloc") (param i32) (result i32)
    i32.const 1000
  )
  
  (func (export "seraph_free") (param i32 i32)
    nop
  )

  ;; Returns pointer 65530 (almost at end of 1 page which is 65536) + length 10.
  ;; 65530 + 10 = 65540 > 65536 (OOB)
  (func (export "print") (param $ptr i32) (param $len i32) (result i64)
    (i64.or
      (i64.shl (i64.const 65530) (i64.const 32)) ;; ptr
      (i64.const 10)                             ;; len
    )
  )
)
