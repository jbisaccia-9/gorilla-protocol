package com.gorillagoldeneye.game;

public final class Player {
    private double x = 2.5;
    private double y = 2.5;
    private double angle = 0.0;
    private int health = GameConstants.STARTING_HEALTH;
    private int ammo = GameConstants.MAX_AMMO;
    private int score = 0;

    public void reset() {
        x = 2.5;
        y = 2.5;
        angle = 0.0;
        health = GameConstants.STARTING_HEALTH;
        ammo = GameConstants.MAX_AMMO;
        score = 0;
    }

    public double getX() {
        return x;
    }

    public void setX(double x) {
        this.x = x;
    }

    public double getY() {
        return y;
    }

    public void setY(double y) {
        this.y = y;
    }

    public double getAngle() {
        return angle;
    }

    public void rotate(double delta) {
        angle += delta;
    }

    public int getHealth() {
        return health;
    }

    public void damage(int amount) {
        health -= amount;
    }

    public void heal(int amount) {
        health = Math.min(GameConstants.STARTING_HEALTH, health + amount);
    }

    public void setHealth(int health) {
        this.health = health;
    }

    public int getAmmo() {
        return ammo;
    }

    public void setAmmo(int ammo) {
        this.ammo = ammo;
    }

    public void addAmmo(int amount) {
        ammo = Math.min(GameConstants.MAX_AMMO, ammo + amount);
    }

    public int getScore() {
        return score;
    }

    public void addScore(int amount) {
        score += amount;
    }
}
