# Cognitive Frame JSON Contract

The Warden validates all outputs against strict schemas represented by the `intent_type`. This format operates as the system's foundational LLM grammar. Only these four exact structures are permitted. 

## 1. System2Think
Used for the agent's internal monologue and chain of thought processing.
```json
{
  "frame_id": 1,
  "timestamp_ms": 1712200000,
  "intent_type": "System2Think",
  "payload": {
    "internal_monologue": "Let me construct a data path..."
  }
}
```

## 2. QueryMerovingian
Used to query the node graph or relationship properties.
```json
{
  "frame_id": 2,
  "timestamp_ms": 1712200010,
  "intent_type": "QueryMerovingian",
  "payload": {
    "entity_node_id": "user_42",
    "relation_type": "KNOWS"
  }
}
```

## 3. InvokeSeraph
Used to execute logic inside web assembly sandboxes.
```json
{
  "frame_id": 3,
  "timestamp_ms": 1712200020,
  "intent_type": "InvokeSeraph",
  "payload": {
    "target_wasm_module": "math_helper.wasm",
    "arguments": {
      "operation": "add",
      "a": "5",
      "b": "10"
    }
  }
}
```

## 4. BroadcastSmith
Used to transmit opaque binary payload messages to sibling agents.
```json
{
  "frame_id": 4,
  "timestamp_ms": 1712200030,
  "intent_type": "BroadcastSmith",
  "payload": {
    "target_agent_id": 99,
    "binary_payload": [72, 101, 108, 108, 111]
  }
}
```
