-- Drop the table if it already exists (useful for starting fresh)
DROP TABLE IF EXISTS mqtt_devices;

-- Create the table with all fields, including the new 'name' column
CREATE TABLE mqtt_devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT(32),
    cmnd_topic TEXT(64),
    topic TEXT(64) UNIQUE,
    msg TEXT(32),
    msg_type TEXT(1),
    dirty INTEGER,
    truth TEXT(8)
);

-- Create an index on the cmnd_topic column for faster lookups
CREATE INDEX idx_mqtt_cmnd_topic ON mqtt_devices(cmnd_topic);

-- Insert the sample record including the new 'name' field
INSERT INTO mqtt_devices (name, cmnd_topic, topic, msg, msg_type, dirty, truth) 
VALUES ('Jacuzzi',  '/home/Jacuzzi/cmnd/POWER', '/home/Jacuzzi/Power', 'Off', 'B', 0, 'On');

