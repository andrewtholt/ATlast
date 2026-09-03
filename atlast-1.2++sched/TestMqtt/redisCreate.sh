

HSET mqtt:device1 cmnd_topic "/home/Jacuzzi/cmnd/POWER" topic "/home/Jacuzzi/Power" msg "Off" msg_type "B" dirty 0 truth "On"

FT.CREATE idx:mqtt ON HASH PREFIX 1 "mqtt:" SCHEMA cmnd_topic TEXT topic TEXT msg TEXT msg_type TAG dirty NUMERIC truth TEXT

EVAL "local key = redis.call('GET', KEYS[1]); if key then return redis.call('HGET', key, ARGV[1]) else return nil end" 1 "mqtt:topic:/home/Jacuzzi/Power" cmnd_topic

