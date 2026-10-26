package io.coolbox.vision;
/** Image color layout (Python: {@code computer_vision.ImageFormat}). */
public enum ImageFormat { GRAYSCALE(0), RGB(1), RGBA(2); final int value; ImageFormat(int v){value=v;} }
