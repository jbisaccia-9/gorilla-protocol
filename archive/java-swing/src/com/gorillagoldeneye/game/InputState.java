package com.gorillagoldeneye.game;

public final class InputState {
    private boolean moveForward;
    private boolean moveBackward;
    private boolean strafeLeft;
    private boolean strafeRight;
    private boolean turnLeft;
    private boolean turnRight;
    private boolean shootPressed;
    private boolean reloadPressed;
    private boolean restartPressed;
    private double pendingMouseTurn;

    public boolean isMoveForward() {
        return moveForward;
    }

    public void setMoveForward(boolean moveForward) {
        this.moveForward = moveForward;
    }

    public boolean isMoveBackward() {
        return moveBackward;
    }

    public void setMoveBackward(boolean moveBackward) {
        this.moveBackward = moveBackward;
    }

    public boolean isStrafeLeft() {
        return strafeLeft;
    }

    public void setStrafeLeft(boolean strafeLeft) {
        this.strafeLeft = strafeLeft;
    }

    public boolean isStrafeRight() {
        return strafeRight;
    }

    public void setStrafeRight(boolean strafeRight) {
        this.strafeRight = strafeRight;
    }

    public boolean isTurnLeft() {
        return turnLeft;
    }

    public void setTurnLeft(boolean turnLeft) {
        this.turnLeft = turnLeft;
    }

    public boolean isTurnRight() {
        return turnRight;
    }

    public void setTurnRight(boolean turnRight) {
        this.turnRight = turnRight;
    }

    public boolean isShootPressed() {
        return shootPressed;
    }

    public void setShootPressed(boolean shootPressed) {
        this.shootPressed = shootPressed;
    }

    public boolean isReloadPressed() {
        return reloadPressed;
    }

    public void setReloadPressed(boolean reloadPressed) {
        this.reloadPressed = reloadPressed;
    }

    public boolean isRestartPressed() {
        return restartPressed;
    }

    public void setRestartPressed(boolean restartPressed) {
        this.restartPressed = restartPressed;
    }

    public void addMouseTurn(double delta) {
        pendingMouseTurn += delta;
    }

    public double consumeMouseTurn() {
        double delta = pendingMouseTurn;
        pendingMouseTurn = 0.0;
        return delta;
    }
}
