package com.gorillagoldeneye.render;

import com.gorillagoldeneye.game.Enemy;
import com.gorillagoldeneye.game.GameConstants;
import com.gorillagoldeneye.game.GameWorld;
import com.gorillagoldeneye.game.LevelMap;
import com.gorillagoldeneye.game.Pickup;
import com.gorillagoldeneye.game.Player;
import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public final class Renderer {
    private final BufferedImage wallTexture = TextureFactory.createWallTexture();
    private final BufferedImage floorTexture = TextureFactory.createFloorTexture();
    private final BufferedImage ceilingTexture = TextureFactory.createCeilingTexture();

    public FrameData render(Graphics2D g, GameWorld world) {
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        renderBackground(g);
        FrameData frameData = new FrameData(GameConstants.SCREEN_WIDTH);
        renderWalls(g, world, frameData);
        renderPickups(g, world, frameData);
        renderEnemies(g, world, frameData);
        renderWeapon(g);
        renderHud(g, world);
        if (world.isGameOver() || world.isVictory()) {
            renderOverlay(g, world);
        }
        return frameData;
    }

    private void renderBackground(Graphics2D g) {
        int half = GameConstants.SCREEN_HEIGHT / 2;
        for (int y = 0; y < half; y++) {
            int texY = y % GameConstants.TEXTURE_SIZE;
            for (int x = 0; x < GameConstants.SCREEN_WIDTH; x += 4) {
                int texX = x % GameConstants.TEXTURE_SIZE;
                g.setColor(new Color(ceilingTexture.getRGB(texX, texY)));
                g.fillRect(x, y, 4, 1);
                g.setColor(new Color(floorTexture.getRGB(texX, texY)));
                g.fillRect(x, y + half, 4, 1);
            }
        }
    }

    private void renderWalls(Graphics2D g, GameWorld world, FrameData frameData) {
        Player player = world.getPlayer();
        LevelMap map = world.getMap();
        double[] depthBuffer = frameData.getDepthBuffer();
        for (int x = 0; x < GameConstants.SCREEN_WIDTH; x++) {
            double rayAngle = player.getAngle() - GameConstants.FOV / 2.0
                + GameConstants.FOV * x / GameConstants.SCREEN_WIDTH;
            double rayDirX = Math.cos(rayAngle);
            double rayDirY = Math.sin(rayAngle);
            double distance = 0.0;
            double sampleX = player.getX();
            double sampleY = player.getY();
            while (distance < GameConstants.MAX_VIEW_DISTANCE) {
                distance += 0.02;
                sampleX = player.getX() + rayDirX * distance;
                sampleY = player.getY() + rayDirY * distance;
                if (map.isWall(sampleX, sampleY)) {
                    break;
                }
            }

            double correctedDistance = distance * Math.cos(rayAngle - player.getAngle());
            depthBuffer[x] = correctedDistance;
            int wallHeight = (int) (GameConstants.SCREEN_HEIGHT / Math.max(correctedDistance, 0.0001));
            int wallTop = GameConstants.SCREEN_HEIGHT / 2 - wallHeight / 2;

            double hitX = sampleX - Math.floor(sampleX);
            double hitY = sampleY - Math.floor(sampleY);
            double textureAnchor = hitX > hitY ? hitX : hitY;
            int textureX = (int) (textureAnchor * (GameConstants.TEXTURE_SIZE - 1));

            for (int y = Math.max(0, wallTop); y < Math.min(GameConstants.SCREEN_HEIGHT, wallTop + wallHeight); y++) {
                int textureY = (int) ((double) (y - wallTop) / wallHeight * (GameConstants.TEXTURE_SIZE - 1));
                Color color = new Color(wallTexture.getRGB(textureX, textureY));
                double shade = Math.max(0.25, 1.0 - correctedDistance / GameConstants.MAX_VIEW_DISTANCE);
                int red = (int) (color.getRed() * shade);
                int green = (int) (color.getGreen() * shade);
                int blue = (int) (color.getBlue() * shade);
                g.setColor(new Color(red, green, blue));
                g.drawLine(x, y, x, y);
            }
        }
    }

    private void renderEnemies(Graphics2D g, GameWorld world, FrameData frameData) {
        Player player = world.getPlayer();
        List<EnemyProjection> projections = new ArrayList<>();
        for (Enemy enemy : world.getEnemies()) {
            double dx = enemy.getX() - player.getX();
            double dy = enemy.getY() - player.getY();
            double distance = Math.hypot(dx, dy);
            double angleDiff = normalizeAngle(Math.atan2(dy, dx) - player.getAngle());
            if (Math.abs(angleDiff) < GameConstants.FOV / 1.5 && distance > 0.1) {
                projections.add(new EnemyProjection(enemy, distance, angleDiff));
            }
        }
        Collections.sort(projections, new Comparator<EnemyProjection>() {
            @Override
            public int compare(EnemyProjection a, EnemyProjection b) {
                return Double.compare(b.distance, a.distance);
            }
        });

        for (EnemyProjection projection : projections) {
            int spriteSize = (int) (GameConstants.SCREEN_HEIGHT / projection.distance);
            int screenX = (int) ((projection.angleDiff + GameConstants.FOV / 2.0)
                / GameConstants.FOV * GameConstants.SCREEN_WIDTH) - spriteSize / 2;
            int screenY = GameConstants.SCREEN_HEIGHT / 2 - spriteSize / 2;
            drawEnemy(g, projection.enemy, projection.distance, frameData.getDepthBuffer(), screenX, screenY, spriteSize);
        }
    }

    private void renderPickups(Graphics2D g, GameWorld world, FrameData frameData) {
        Player player = world.getPlayer();
        List<PickupProjection> projections = new ArrayList<>();
        for (Pickup pickup : world.getPickups()) {
            double dx = pickup.getX() - player.getX();
            double dy = pickup.getY() - player.getY();
            double distance = Math.hypot(dx, dy);
            double angleDiff = normalizeAngle(Math.atan2(dy, dx) - player.getAngle());
            if (Math.abs(angleDiff) < GameConstants.FOV / 1.5 && distance > 0.1) {
                projections.add(new PickupProjection(pickup, distance, angleDiff));
            }
        }
        Collections.sort(projections, new Comparator<PickupProjection>() {
            @Override
            public int compare(PickupProjection a, PickupProjection b) {
                return Double.compare(b.distance, a.distance);
            }
        });

        for (PickupProjection projection : projections) {
            int spriteSize = (int) (GameConstants.SCREEN_HEIGHT / projection.distance / 3.0);
            int screenX = (int) ((projection.angleDiff + GameConstants.FOV / 2.0)
                / GameConstants.FOV * GameConstants.SCREEN_WIDTH) - spriteSize / 2;
            int screenY = GameConstants.SCREEN_HEIGHT / 2 + spriteSize / 3;
            drawPickup(g, projection.pickup, projection.distance, frameData.getDepthBuffer(), screenX, screenY, spriteSize);
        }
    }

    private void drawPickup(Graphics2D g, Pickup pickup, double distance, double[] depthBuffer,
                            int screenX, int screenY, int spriteSize) {
        for (int x = 0; x < spriteSize; x++) {
            int drawX = screenX + x;
            if (drawX < 0 || drawX >= GameConstants.SCREEN_WIDTH || distance >= depthBuffer[drawX]) {
                continue;
            }

            for (int y = 0; y < spriteSize; y++) {
                int drawY = screenY + y;
                if (drawY < 0 || drawY >= GameConstants.SCREEN_HEIGHT) {
                    continue;
                }
                double normalizedX = (double) x / spriteSize;
                double normalizedY = (double) y / spriteSize;
                Color color = samplePickupColor(normalizedX, normalizedY, pickup.getType());
                if (color != null) {
                    g.setColor(color);
                    g.drawLine(drawX, drawY, drawX, drawY);
                }
            }
        }
    }

    private Color samplePickupColor(double x, double y, Pickup.Type type) {
        if (type == Pickup.Type.BANANA_AMMO) {
            double curve = 0.55 + Math.sin(x * Math.PI) * 0.18;
            if (Math.abs(y - curve) < 0.14 && x > 0.12 && x < 0.88) {
                return new Color(245, 210, 45);
            }
        } else if (type == Pickup.Type.MEDKIT) {
            if (x > 0.18 && x < 0.82 && y > 0.18 && y < 0.82) {
                if ((x > 0.43 && x < 0.57) || (y > 0.43 && y < 0.57)) {
                    return new Color(210, 20, 32);
                }
                return Color.WHITE;
            }
        } else {
            if (x > 0.22 && x < 0.78 && y > 0.14 && y < 0.86) {
                return new Color(80, 210, 210);
            }
            if (x > 0.35 && x < 0.65 && y > 0.3 && y < 0.7) {
                return new Color(20, 70, 90);
            }
        }
        return null;
    }

    private void drawEnemy(Graphics2D g, Enemy enemy, double distance, double[] depthBuffer,
                           int screenX, int screenY, int spriteSize) {
        for (int x = 0; x < spriteSize; x++) {
            int drawX = screenX + x;
            if (drawX < 0 || drawX >= GameConstants.SCREEN_WIDTH || distance >= depthBuffer[drawX]) {
                continue;
            }

            for (int y = 0; y < spriteSize; y++) {
                int drawY = screenY + y;
                if (drawY < 0 || drawY >= GameConstants.SCREEN_HEIGHT) {
                    continue;
                }

                double normalizedX = (double) x / spriteSize;
                double normalizedY = (double) y / spriteSize;
                Color color = sampleEnemyColor(normalizedX, normalizedY, enemy.getHealth());
                if (color != null) {
                    g.setColor(color);
                    g.drawLine(drawX, drawY, drawX, drawY);
                }
            }
        }
    }

    private Color sampleEnemyColor(double x, double y, int health) {
        if (y < 0.28 && x > 0.3 && x < 0.7) {
            return new Color(210, 180, 130);
        }
        if (y >= 0.22 && y < 0.75 && x > 0.22 && x < 0.78) {
            return new Color(80, 88, 112);
        }
        if (y >= 0.75 && y < 0.95 && ((x > 0.28 && x < 0.42) || (x > 0.58 && x < 0.72))) {
            return new Color(50, 50, 62);
        }
        if (y >= 0.35 && y < 0.55 && ((x > 0.1 && x < 0.24) || (x > 0.76 && x < 0.9))) {
            return new Color(65, 65, 80);
        }
        if (y < 0.08 && x > 0.2 && x < 0.8) {
            return health > 1 ? Color.GREEN : Color.RED;
        }
        return null;
    }

    private void renderWeapon(Graphics2D g) {
        int centerX = GameConstants.SCREEN_WIDTH / 2;
        int baseY = GameConstants.SCREEN_HEIGHT - 78;
        g.setColor(new Color(112, 72, 38));
        g.fillRoundRect(centerX - 120, baseY - 24, 240, 76, 25, 25);
        g.setColor(new Color(166, 118, 70));
        g.fillRoundRect(centerX - 26, baseY - 100, 52, 132, 22, 22);
        g.setColor(new Color(225, 210, 120));
        g.fillOval(centerX - 12, GameConstants.SCREEN_HEIGHT / 2 - 12, 24, 24);
        g.setColor(Color.BLACK);
        g.drawOval(centerX - 12, GameConstants.SCREEN_HEIGHT / 2 - 12, 24, 24);
    }

    private void renderHud(Graphics2D g, GameWorld world) {
        Player player = world.getPlayer();
        g.setFont(new Font("Monospaced", Font.BOLD, 20));
        g.setColor(Color.WHITE);
        g.drawString("SALUTE: " + player.getHealth(), 20, 30);
        g.drawString("MUNIZIONI: " + player.getAmmo(), 20, 58);
        g.drawString("PUNTEGGIO: " + player.getScore(), 20, 86);
        g.drawString("LIVELLO: " + world.getLevelNumber(), 20, 114);
        g.drawString("DOCUMENTI: " + (world.hasIntel() ? "SI" : "NO"), 20, 142);

        g.setColor(new Color(0, 0, 0, 170));
        g.fillRoundRect(20, GameConstants.SCREEN_HEIGHT - 92, GameConstants.SCREEN_WIDTH - 40, 58, 18, 18);
        g.setColor(new Color(255, 230, 120));
        g.setFont(new Font("SansSerif", Font.BOLD, 22));
        g.drawString("Gori Kongo: \"" + world.getCurrentDialogue() + "\"", 35, GameConstants.SCREEN_HEIGHT - 54);

        g.setFont(new Font("Monospaced", Font.PLAIN, 16));
        g.setColor(Color.WHITE);
        g.drawString("WASD muovi | Frecce gira | Mouse guarda | Spazio spara | R ricarica/ricomincia",
            20, GameConstants.SCREEN_HEIGHT - 15);
    }

    private void renderOverlay(Graphics2D g, GameWorld world) {
        g.setColor(new Color(0, 0, 0, 180));
        g.fillRect(0, 0, GameConstants.SCREEN_WIDTH, GameConstants.SCREEN_HEIGHT);
        g.setColor(world.isVictory() ? Color.GREEN : Color.RED);
        g.setFont(new Font("SansSerif", Font.BOLD, 46));
        g.drawString(world.isVictory() ? "VITTORIA GORILLESCA" : "MISSIONE FALLITA", 220, 220);
        g.setColor(Color.WHITE);
        g.setFont(new Font("SansSerif", Font.PLAIN, 26));
        g.drawString(world.getCurrentDialogue(), 180, 280);
        g.drawString("Premi R per giocare ancora.", 305, 340);
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

    private static final class EnemyProjection {
        private final Enemy enemy;
        private final double distance;
        private final double angleDiff;

        private EnemyProjection(Enemy enemy, double distance, double angleDiff) {
            this.enemy = enemy;
            this.distance = distance;
            this.angleDiff = angleDiff;
        }
    }

    private static final class PickupProjection {
        private final Pickup pickup;
        private final double distance;
        private final double angleDiff;

        private PickupProjection(Pickup pickup, double distance, double angleDiff) {
            this.pickup = pickup;
            this.distance = distance;
            this.angleDiff = angleDiff;
        }
    }
}
