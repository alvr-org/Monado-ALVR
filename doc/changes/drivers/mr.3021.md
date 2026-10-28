d/euroc, st/prober: Size the SLAM tracker by the cameras `EUROC_CAM_COUNT`
actually plays back instead of the dataset's full camera count, so playback no
longer stalls waiting for a frame from a camera it never streams.
