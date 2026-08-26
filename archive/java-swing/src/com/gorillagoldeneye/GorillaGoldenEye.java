package com.gorillagoldeneye;

import com.gorillagoldeneye.ui.GamePanel;
import javax.swing.JFrame;
import javax.swing.SwingUtilities;

public final class GorillaGoldenEye {
    private GorillaGoldenEye() {
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                JFrame frame = new JFrame("Gorilla Golden Eye");
                frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
                frame.setResizable(false);
                frame.add(new GamePanel());
                frame.pack();
                frame.setLocationRelativeTo(null);
                frame.setVisible(true);
            }
        });
    }
}
