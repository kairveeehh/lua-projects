local mysql = require("mysqlprep")

-- Connect to MySQL database
local conn = mysql.connect("localhost", "root", "jampu@2004", "testdb")

-- Create a table if it doesn't exist
local create_stmt = mysql.prepare(conn, "CREATE TABLE IF NOT EXISTS users (id INT AUTO_INCREMENT PRIMARY KEY, age INT)")
mysql.execute(create_stmt)

-- Insert data using prepared statement and bind
local insert_stmt = mysql.prepare(conn, "INSERT INTO users (age) VALUES (?)")
mysql.bind(insert_stmt, 25)  
mysql.execute(insert_stmt)

mysql.bind(insert_stmt, 30)  
mysql.execute(insert_stmt)

print("Data inserted successfully")

-- Select data
local select_stmt = mysql.prepare(conn, "SELECT age FROM users")
mysql.execute(select_stmt)

-- Fetch the result
print("Fetching data:")
local age_list = mysql.fetch(select_stmt)

while age_list do
    if type(age_list) == "table" then
        print(table.concat(age_list, ", "))  -- Works if fetch returns a table
    else
        print(age_list)  -- Works if fetch returns a single value
    end
    age_list = mysql.fetch(select_stmt)
end
