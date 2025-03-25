-- mysqlpool.lua
local mysqlprep = require("mysqlprep")

local mysqlpool = {}
mysqlpool.__index = mysqlpool

function mysqlpool.new(config)
    local self = setmetatable({}, mysqlpool)
    self.pool = {}
    self.config = config
    local pool_size = config.pool_size or 5  -- Default pool size if not provided

    for i = 1, pool_size do
        local conn = mysqlprep.connect(
            config.host,
            config.user,
            config.password,
            config.database
        )
        if conn then
            table.insert(self.pool, conn)
        else
            print("Failed to create a connection for pool index:", i)
        end
    end

    return self
end

function mysqlpool:get()
    return table.remove(self.pool)
end

function mysqlpool:put(conn)
    table.insert(self.pool, conn)
end

return mysqlpool

