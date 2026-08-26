package com.gorillagoldeneye.render;

public final class FrameData {
    private final double[] depthBuffer;

    public FrameData(int width) {
        this.depthBuffer = new double[width];
    }

    public double[] getDepthBuffer() {
        return depthBuffer;
    }
}
