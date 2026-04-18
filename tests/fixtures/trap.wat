(module
  (memory (export "memory") 1)
  
  (func (export "seraph_alloc") (param i32) (result i32)
    i32.const 1000
  )
  
  (func (export "seraph_free") (param i32 i32)
    nop
  )

  (func (export "print") (param $ptr i32) (param $len i32) (result i64)
    ;; trigger an unreachable trap immediately upon execution
    unreachable
    (i64.const 0)
  )
)
