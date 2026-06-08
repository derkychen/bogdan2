import numpy as np

def get_peak_intensity(data_mV, intensity_channel_name, pre_trigger_samples):
   """
   Returns the maximum post-trigger intensity voltage.
   """

   waveform = np.array(data_mV[intensity_channel_name])
   return float(np.max(waveform[pre_trigger_samples:]))

def get_average_position(data_mV, x_channel_name, y_channel_name, pre_trigger_samples):
   """
   Averages X and Y position voltage after the trigger.

   Later, convert these voltages to real distance using calibration:
   """

   x_waveform = np.array(data_mV[x_channel_name])
   y_waveform = np.array(data_mV[y_channel_name])

   x_position_mV = float(np.mean(x_waveform[pre_trigger_samples:]))
   y_position_mV = float(np.mean(y_waveform[pre_trigger_samples:]))

   return x_position_mV, y_position_mV

def make_beam_point(
   data_mV,
   pre_trigger_samples,
   intensity_channel_name="intensity",
   x_channel_name="x_position",
   y_channel_name="y_position"
):
   peak_intensity = get_peak_intensity(
       data_mV,
       intensity_channel_name,
       pre_trigger_samples
   )

   x_position_mV, y_position_mV = get_average_position(
       data_mV,
       x_channel_name,
       y_channel_name,
       pre_trigger_samples
   )

   return {
       "x_position_mV": x_position_mV,
       "y_position_mV": y_position_mV,
       "peak_intensity_mV": peak_intensity,
   }

def average_beam_points(points):
   """
   Averages several single-pulse beam point dictionaries.
   """

   return {
       "x_position_mV": float(np.mean([p["x_position_mV"] for p in points])),
       "y_position_mV": float(np.mean([p["y_position_mV"] for p in points])),
       "peak_intensity_mV": float(np.mean([p["peak_intensity_mV"] for p in points])),
   }





