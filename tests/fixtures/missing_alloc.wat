(module
  (memory (export "memory") 1)
  
  ;; Missing seraph_alloc
  
  (func (export "seraph_free") (param i32 i32)
    nop
  )

  (func (export "print") (param $ptr i32) (param $len i32) (result i64)
    (i64.const 0)
  )
)
