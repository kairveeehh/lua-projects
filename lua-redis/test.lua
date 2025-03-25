local redis = require "myredis"

redis.connect("127.0.0.1", 6379)

print("SET:", redis.command("SET key hello"))
print("GET:", redis.command("GET key"))

-- Pipelining
redis.appendCommand("SET foo bar")
redis.appendCommand("INCR counter")
redis.appendCommand("INCR counter")
redis.appendCommand("GET counter")

for i = 1, 4 do
    local reply = redis.getReply()
    print("Pipeline Reply " .. i, reply)
end
