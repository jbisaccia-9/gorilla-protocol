package com.gorillagoldeneye.game;

public final class Pickup {
    public enum Type {
        BANANA_AMMO,
        MEDKIT,
        INTEL
    }

    private final double x;
    private final double y;
    private final Type type;

    public Pickup(double x, double y, Type type) {
        this.x = x;
        this.y = y;
        this.type = type;
    }

    public double getX() {
        return x;
    }

    public double getY() {
        return y;
    }

    public Type getType() {
        return type;
    }
}
