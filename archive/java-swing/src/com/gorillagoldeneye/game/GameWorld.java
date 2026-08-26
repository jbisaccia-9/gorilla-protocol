package com.gorillagoldeneye.game;

import com.gorillagoldeneye.audio.SoundManager;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public final class GameWorld {
    private final LevelMap map = new LevelMap(0);
    private final Player player = new Player();
    private final List<Enemy> enemies = new ArrayList<>();
    private final List<Pickup> pickups = new ArrayList<>();
    private final Random random = new Random();
    private final SoundManager soundManager;
    private final String[] dialoguePool = {
        "Andiamo, banana alla mano!",
        "Sono il gorilla piu elegante della giungla.",
        "Nessuno ferma la mia furia pelosa!",
        "Questa missione profuma di vittoria e banane.",
        "Attento, scimmione in arrivo!",
        "Mamma mia, che mira bestiale!",
        "La mia cravatta invisibile e perfetta.",
        "Qui comando io, in puro italiano."
    };

    private String currentDialogue = "Benvenuto, sono Gori Kongo. Parlo solo italiano.";
    private int dialogueTimer = GameConstants.DIALOGUE_FRAMES;
    private int shotCooldown = 0;
    private int levelIndex = 0;
    private boolean hasIntel = false;
    private boolean gameOver = false;
    private boolean victory = false;

    public GameWorld(SoundManager soundManager) {
        this.soundManager = soundManager;
        spawnPickups();
        spawnEnemies();
    }

    public void update(InputState input) {
        if (gameOver || victory) {
            if (input.isRestartPressed()) {
                reset();
            }
            return;
        }

        handleRotation(input);
        handleMovement(input);
        handlePickups();
        handleCombat(input);
        updateEnemies();
        tickDialogue();

        if (player.getHealth() <= 0) {
            gameOver = true;
            currentDialogue = "Mi arrendo... ma solo per adesso.";
            soundManager.playGameOver();
        } else if (enemies.isEmpty() && hasIntel) {
            completeLevel();
        }
    }

    private void handleRotation(InputState input) {
        if (input.isTurnLeft()) {
            player.rotate(-GameConstants.ROT_SPEED);
        }
        if (input.isTurnRight()) {
            player.rotate(GameConstants.ROT_SPEED);
        }
        player.rotate(input.consumeMouseTurn() * GameConstants.MOUSE_SENSITIVITY);
    }

    private void handleMovement(InputState input) {
        double angle = player.getAngle();
        double moveX = 0.0;
        double moveY = 0.0;
        if (input.isMoveForward()) {
            moveX += Math.cos(angle) * GameConstants.MOVE_SPEED;
            moveY += Math.sin(angle) * GameConstants.MOVE_SPEED;
        }
        if (input.isMoveBackward()) {
            moveX -= Math.cos(angle) * GameConstants.MOVE_SPEED;
            moveY -= Math.sin(angle) * GameConstants.MOVE_SPEED;
        }
        if (input.isStrafeLeft()) {
            moveX += Math.cos(angle - Math.PI / 2.0) * GameConstants.STRAFE_SPEED;
            moveY += Math.sin(angle - Math.PI / 2.0) * GameConstants.STRAFE_SPEED;
        }
        if (input.isStrafeRight()) {
            moveX += Math.cos(angle + Math.PI / 2.0) * GameConstants.STRAFE_SPEED;
            moveY += Math.sin(angle + Math.PI / 2.0) * GameConstants.STRAFE_SPEED;
        }

        tryMove(player.getX() + moveX, player.getY());
        tryMove(player.getX(), player.getY() + moveY);
    }

    private void tryMove(double nextX, double nextY) {
        if (!map.isWall(nextX, nextY)) {
            player.setX(nextX);
            player.setY(nextY);
        }
    }

    private void handleCombat(InputState input) {
        if (shotCooldown > 0) {
            shotCooldown--;
        }

        if (input.isReloadPressed() && player.getAmmo() < GameConstants.MAX_AMMO) {
            player.setAmmo(GameConstants.MAX_AMMO);
            currentDialogue = "Ricarica completata. Eleganza italiana.";
            dialogueTimer = GameConstants.DIALOGUE_FRAMES;
            soundManager.playReload();
        }

        if (!input.isShootPressed() || shotCooldown != 0) {
            return;
        }

        shotCooldown = 12;
        soundManager.playShot();
        if (player.getAmmo() == 0) {
            currentDialogue = "Click! Mi servono banane... ehm, munizioni.";
            dialogueTimer = GameConstants.DIALOGUE_FRAMES;
            return;
        }

        player.setAmmo(player.getAmmo() - 1);
        Enemy hitEnemy = null;
        double bestAlignment = 0.985;
        for (Enemy enemy : enemies) {
            double dx = enemy.getX() - player.getX();
            double dy = enemy.getY() - player.getY();
            double distance = Math.hypot(dx, dy);
            double angleToEnemy = Math.atan2(dy, dx);
            double angleDiff = normalizeAngle(angleToEnemy - player.getAngle());
            double alignment = Math.cos(angleDiff);
            if (distance < 8.0 && alignment > bestAlignment && hasLineOfSight(enemy.getX(), enemy.getY())) {
                bestAlignment = alignment;
                hitEnemy = enemy;
            }
        }

        if (hitEnemy == null) {
            currentDialogue = "Mancato! La prossima sara perfetta.";
            dialogueTimer = 90;
            return;
        }

        hitEnemy.damage();
        currentDialogue = "Colpito! Che stile primatesco!";
        dialogueTimer = 90;
        if (hitEnemy.getHealth() <= 0) {
            enemies.remove(hitEnemy);
            player.addScore(100);
            player.addAmmo(1);
            soundManager.playEnemyDown();
        }
    }

    private void updateEnemies() {
        for (Enemy enemy : enemies) {
            double dx = player.getX() - enemy.getX();
            double dy = player.getY() - enemy.getY();
            double distance = Math.hypot(dx, dy);
            if (distance > 0.8) {
                double stepX = dx / distance * 0.025;
                double stepY = dy / distance * 0.025;
                double nextX = enemy.getX() + stepX;
                double nextY = enemy.getY() + stepY;
                if (!map.isWall(nextX, enemy.getY())) {
                    enemy.setX(nextX);
                }
                if (!map.isWall(enemy.getX(), nextY)) {
                    enemy.setY(nextY);
                }
            } else {
                enemy.setAttackCooldown(enemy.getAttackCooldown() - 1);
                if (enemy.getAttackCooldown() <= 0) {
                    player.damage(8);
                    enemy.setAttackCooldown(30);
                    currentDialogue = "Ahi! Questo scagnozzo mi ha spettinato.";
                    dialogueTimer = 90;
                    soundManager.playDamage();
                }
            }
        }
    }

    private void handlePickups() {
        for (int i = pickups.size() - 1; i >= 0; i--) {
            Pickup pickup = pickups.get(i);
            double distance = Math.hypot(pickup.getX() - player.getX(), pickup.getY() - player.getY());
            if (distance > 0.55) {
                continue;
            }

            if (pickup.getType() == Pickup.Type.BANANA_AMMO) {
                player.addAmmo(4);
                currentDialogue = "Banane tattiche raccolte. Munizioni pronte!";
            } else if (pickup.getType() == Pickup.Type.MEDKIT) {
                player.heal(28);
                currentDialogue = "Pronto soccorso gorillesco. Sto benissimo.";
            } else {
                hasIntel = true;
                currentDialogue = "Documenti presi. Tutto chiarissimo, in italiano.";
            }
            dialogueTimer = GameConstants.DIALOGUE_FRAMES;
            pickups.remove(i);
            soundManager.playPickup();
        }
    }

    private void tickDialogue() {
        if (dialogueTimer > 0) {
            dialogueTimer--;
        } else if (random.nextDouble() < 0.01) {
            currentDialogue = dialoguePool[random.nextInt(dialoguePool.length)];
            dialogueTimer = GameConstants.DIALOGUE_FRAMES;
        }
    }

    private boolean hasLineOfSight(double targetX, double targetY) {
        double dx = targetX - player.getX();
        double dy = targetY - player.getY();
        double distance = Math.hypot(dx, dy);
        int steps = Math.max(1, (int) (distance * 12));
        for (int i = 1; i <= steps; i++) {
            double sampleX = player.getX() + dx * i / steps;
            double sampleY = player.getY() + dy * i / steps;
            if (map.isWall(sampleX, sampleY)) {
                return false;
            }
        }
        return true;
    }

    private double normalizeAngle(double angle) {
        while (angle < -Math.PI) {
            angle += Math.PI * 2;
        }
        while (angle > Math.PI) {
            angle -= Math.PI * 2;
        }
        return angle;
    }

    private void reset() {
        enemies.clear();
        pickups.clear();
        player.reset();
        shotCooldown = 0;
        levelIndex = 0;
        hasIntel = false;
        map.setLevelIndex(levelIndex);
        dialogueTimer = GameConstants.DIALOGUE_FRAMES;
        currentDialogue = "Si ricomincia, piu gorilla di prima.";
        gameOver = false;
        victory = false;
        spawnPickups();
        spawnEnemies();
    }

    private void completeLevel() {
        if (levelIndex >= map.getLevelCount() - 1) {
            victory = true;
            currentDialogue = "Missione compiuta! La giungla canta in italiano!";
            soundManager.playVictory();
            return;
        }

        levelIndex++;
        map.setLevelIndex(levelIndex);
        enemies.clear();
        pickups.clear();
        player.setX(2.5);
        player.setY(2.5);
        hasIntel = false;
        currentDialogue = "Livello superato. Avanti, con passo da gentiluomo.";
        dialogueTimer = GameConstants.DIALOGUE_FRAMES;
        soundManager.playLevelClear();
        spawnPickups();
        spawnEnemies();
    }

    private void spawnEnemies() {
        int enemyTarget = GameConstants.ENEMY_COUNT + levelIndex * 2;
        while (enemies.size() < enemyTarget) {
            double x = 1.5 + random.nextInt(GameConstants.MAP_WIDTH - 2);
            double y = 1.5 + random.nextInt(GameConstants.MAP_HEIGHT - 2);
            if (!map.isWall(x, y) && Math.hypot(x - player.getX(), y - player.getY()) > 3.0) {
                enemies.add(new Enemy(x, y));
            }
        }
    }

    private void spawnPickups() {
        if (levelIndex == 0) {
            addPickup(4.5, 1.5, Pickup.Type.BANANA_AMMO);
            addPickup(14.5, 1.5, Pickup.Type.INTEL);
            addPickup(2.5, 13.5, Pickup.Type.MEDKIT);
            addPickup(10.5, 14.5, Pickup.Type.BANANA_AMMO);
        } else if (levelIndex == 1) {
            addPickup(5.5, 1.5, Pickup.Type.BANANA_AMMO);
            addPickup(14.5, 14.5, Pickup.Type.INTEL);
            addPickup(1.5, 13.5, Pickup.Type.MEDKIT);
            addPickup(12.5, 11.5, Pickup.Type.BANANA_AMMO);
        } else {
            addPickup(5.5, 1.5, Pickup.Type.BANANA_AMMO);
            addPickup(14.5, 14.5, Pickup.Type.INTEL);
            addPickup(1.5, 13.5, Pickup.Type.MEDKIT);
            addPickup(8.5, 14.5, Pickup.Type.BANANA_AMMO);
        }
    }

    private void addPickup(double x, double y, Pickup.Type type) {
        if (!map.isWall(x, y)) {
            pickups.add(new Pickup(x, y, type));
        }
    }

    public LevelMap getMap() {
        return map;
    }

    public Player getPlayer() {
        return player;
    }

    public List<Enemy> getEnemies() {
        return enemies;
    }

    public List<Pickup> getPickups() {
        return pickups;
    }

    public String getCurrentDialogue() {
        return currentDialogue;
    }

    public boolean isGameOver() {
        return gameOver;
    }

    public boolean isVictory() {
        return victory;
    }

    public int getLevelNumber() {
        return levelIndex + 1;
    }

    public boolean hasIntel() {
        return hasIntel;
    }
}
