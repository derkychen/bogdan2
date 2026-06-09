from picosdk.ps2000a import ps2000a as ps

PICO_CONFIG = {
    "settings_json_path": "instructions/example.json",

    #Set to "auto" if you want the threshold voltage to be calculated
    #Set a number if you want a manual threshold
    
    "trigger_threshold_mode": "manual",
    "manual_trigger_threshold_mV": 1000,
    "sigma_multiplier": 7,

    "trigger_channel_name": "trigger",
    "trigger_direction": "PS2000A_RISING",

    "oversample": 1,
    "timeout_s": 10,

    "channels":{
        "trigger": ps.PS2000A_CHANNEL["PS2000A_CHANNEL_A"],
        "x_position": ps.PS2000A_CHANNEL["PS2000A_CHANNEL_B"],
        "y_position": ps.PS2000A_CHANNEL["PS2000A_CHANNEL_C"],
        "intensity": ps.PS2000A_CHANNEL["PS2000A_CHANNEL_D"]

    },

    "channel_ranges":{
        "trigger": ps.PS2000A_RANGE["PS2000A_10V"],
        "x_position": ps.PS2000A_RANGE["PS2000A_10V"],
        "y_position": ps.PS2000A_RANGE["PS2000A_20V"],
        "intensity": ps.PS2000A_RANGE["PS2000A_10V"]
    }
}