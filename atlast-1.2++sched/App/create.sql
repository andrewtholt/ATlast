-- Drop the table if it already exists (useful for starting fresh)
DROP TABLE IF EXISTS mqtt_devices;

-- Drop the index if it already exists to prevent errors if running multiple times
DROP INDEX IF EXISTS idx_mqtt_topic_pub;

-- Create the table with all fields, including the new 'name' column
CREATE TABLE mqtt_devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT(32) UNIQUE,
    topic_pub TEXT(64),
    topic_sub TEXT(64) UNIQUE,
    msg TEXT(32),
    msg_type TEXT(1), -- Bool|Time|
    dirty INTEGER,
    truth TEXT(8)
);

-- Create an index on the topic_pub column for faster lookups
CREATE INDEX idx_mqtt_topic_pub ON mqtt_devices(topic_pub);

-- Insert the sample record including the new 'name' field
INSERT INTO mqtt_devices (name, topic_pub, topic_sub, msg, msg_type, dirty, truth) 
VALUES ('Jacuzzi',  '/home/Jacuzzi/cmnd/POWER', '/home/Jacuzzi/Power', 'Off', 'B', 0, 'On');

