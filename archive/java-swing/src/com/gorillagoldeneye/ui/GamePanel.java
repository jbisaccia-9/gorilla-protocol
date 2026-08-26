package com.gorillagoldeneye.ui;

import com.gorillagoldeneye.audio.SoundManager;
import com.gorillagoldeneye.game.GameConstants;
import com.gorillagoldeneye.game.GameWorld;
import com.gorillagoldeneye.game.InputState;
import com.gorillagoldeneye.render.Renderer;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionAdapter;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JPanel;
import javax.swing.Timer;

public final class GamePanel extends JPanel {
    private final InputState input = new InputState();
    private final Renderer renderer = new Renderer();
    private final GameWorld world = new GameWorld(new SoundManager());
    private final Timer timer;
    private int lastMouseX = GameConstants.SCREEN_WIDTH / 2;
    private boolean hasMouseReference = false;

    public GamePanel() {
        setPreferredSize(new Dimension(GameConstants.SCREEN_WIDTH, GameConstants.SCREEN_HEIGHT));
        setFocusable(true);
        setBackground(Color.BLACK);
        addKeyListener(new GameKeyHandler());
        addMouseMotionListener(new MouseLookHandler());
        timer = new Timer(GameConstants.FRAME_MS, new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                world.update(input);
                repaint();
            }
        });
        timer.start();
    }

    @Override
    protected void paintComponent(Graphics graphics) {
        super.paintComponent(graphics);
        renderer.render((Graphics2D) graphics, world);
    }

    private final class GameKeyHandler extends KeyAdapter {
        @Override
        public void keyPressed(KeyEvent event) {
            setState(event.getKeyCode(), true);
        }

        @Override
        public void keyReleased(KeyEvent event) {
            setState(event.getKeyCode(), false);
        }

        private void setState(int keyCode, boolean pressed) {
            switch (keyCode) {
                case KeyEvent.VK_W:
                    input.setMoveForward(pressed);
                    break;
                case KeyEvent.VK_S:
                    input.setMoveBackward(pressed);
                    break;
                case KeyEvent.VK_A:
                    input.setStrafeLeft(pressed);
                    break;
                case KeyEvent.VK_D:
                    input.setStrafeRight(pressed);
                    break;
                case KeyEvent.VK_LEFT:
                    input.setTurnLeft(pressed);
                    break;
                case KeyEvent.VK_RIGHT:
                    input.setTurnRight(pressed);
                    break;
                case KeyEvent.VK_SPACE:
                    input.setShootPressed(pressed);
                    break;
                case KeyEvent.VK_R:
                    input.setReloadPressed(pressed);
                    input.setRestartPressed(pressed);
                    break;
                default:
                    break;
            }
        }
    }

    private final class MouseLookHandler extends MouseMotionAdapter {
        @Override
        public void mouseMoved(MouseEvent event) {
            updateMouseTurn(event);
        }

        @Override
        public void mouseDragged(MouseEvent event) {
            updateMouseTurn(event);
        }

        private void updateMouseTurn(MouseEvent event) {
            if (!hasMouseReference) {
                lastMouseX = event.getX();
                hasMouseReference = true;
                return;
            }
            int delta = event.getX() - lastMouseX;
            lastMouseX = event.getX();
            input.addMouseTurn(delta);
        }
    }
}
