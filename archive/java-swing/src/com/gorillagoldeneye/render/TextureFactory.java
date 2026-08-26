package com.gorillagoldeneye.render;

import com.gorillagoldeneye.game.GameConstants;
import java.awt.Color;
import java.awt.image.BufferedImage;

public final class TextureFactory {
    private TextureFactory() {
    }

    public static BufferedImage createWallTexture() {
        int size = GameConstants.TEXTURE_SIZE;
        BufferedImage image = new BufferedImage(size, size, BufferedImage.TYPE_INT_RGB);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int mortar = (x % 16 < 2) || (y % 16 < 2) ? 28 : 0;
                int variation = ((x * 13 + y * 7) % 32);
                Color color = new Color(105 + variation - mortar, 68 + variation / 2 - mortar, 32 + variation / 3 - mortar);
                image.setRGB(x, y, color.getRGB());
            }
        }
        return image;
    }

    public static BufferedImage createFloorTexture() {
        int size = GameConstants.TEXTURE_SIZE;
        BufferedImage image = new BufferedImage(size, size, BufferedImage.TYPE_INT_RGB);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                boolean stripe = ((x / 8) + (y / 8)) % 2 == 0;
                Color color = stripe ? new Color(66, 52, 32) : new Color(52, 40, 24);
                image.setRGB(x, y, color.getRGB());
            }
        }
        return image;
    }

    public static BufferedImage createCeilingTexture() {
        int size = GameConstants.TEXTURE_SIZE;
        BufferedImage image = new BufferedImage(size, size, BufferedImage.TYPE_INT_RGB);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int glow = (int) (40 * Math.sin((x + y) * 0.12) + 40);
                image.setRGB(x, y, new Color(22, 58 + glow / 4, 96 + glow / 3).getRGB());
            }
        }
        return image;
    }
}
