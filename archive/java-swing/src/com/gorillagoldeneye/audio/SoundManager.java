package com.gorillagoldeneye.audio;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.SourceDataLine;

public final class SoundManager {
    private static final float SAMPLE_RATE = 22050.0f;

    public void playShot() {
        playTone(120.0, 0.09, 0.65, Wave.NOISE);
    }

    public void playReload() {
        playTone(520.0, 0.07, 0.35, Wave.SQUARE);
    }

    public void playEnemyDown() {
        playTone(180.0, 0.16, 0.45, Wave.SAW);
    }

    public void playDamage() {
        playTone(90.0, 0.12, 0.55, Wave.SQUARE);
    }

    public void playVictory() {
        playTone(660.0, 0.22, 0.4, Wave.SINE);
    }

    public void playGameOver() {
        playTone(110.0, 0.28, 0.45, Wave.SAW);
    }

    public void playPickup() {
        playTone(840.0, 0.08, 0.35, Wave.SINE);
    }

    public void playLevelClear() {
        playTone(740.0, 0.16, 0.4, Wave.SINE);
    }

    private void playTone(double frequency, double seconds, double volume, Wave wave) {
        Thread thread = new Thread(new ToneTask(frequency, seconds, volume, wave), "gorilla-sound");
        thread.setDaemon(true);
        thread.start();
    }

    private enum Wave {
        SINE,
        SQUARE,
        SAW,
        NOISE
    }

    private static final class ToneTask implements Runnable {
        private final double frequency;
        private final double seconds;
        private final double volume;
        private final Wave wave;

        private ToneTask(double frequency, double seconds, double volume, Wave wave) {
            this.frequency = frequency;
            this.seconds = seconds;
            this.volume = volume;
            this.wave = wave;
        }

        @Override
        public void run() {
            AudioFormat format = new AudioFormat(SAMPLE_RATE, 8, 1, true, false);
            try {
                DataLine.Info info = new DataLine.Info(SourceDataLine.class, format);
                SourceDataLine line = (SourceDataLine) AudioSystem.getLine(info);
                byte[] buffer = createBuffer();
                line.open(format);
                line.start();
                line.write(buffer, 0, buffer.length);
                line.drain();
                line.stop();
                line.close();
            } catch (Exception ignored) {
                // Audio is optional; gameplay should continue if the local device has no output line.
            }
        }

        private byte[] createBuffer() {
            int sampleCount = Math.max(1, (int) (SAMPLE_RATE * seconds));
            byte[] data = new byte[sampleCount];
            long noiseSeed = 0x5EEDL;
            for (int i = 0; i < sampleCount; i++) {
                double progress = (double) i / sampleCount;
                double envelope = 1.0 - progress;
                double phase = 2.0 * Math.PI * frequency * i / SAMPLE_RATE;
                double value;
                if (wave == Wave.SQUARE) {
                    value = Math.sin(phase) >= 0 ? 1.0 : -1.0;
                } else if (wave == Wave.SAW) {
                    value = 2.0 * ((frequency * i / SAMPLE_RATE) % 1.0) - 1.0;
                } else if (wave == Wave.NOISE) {
                    noiseSeed = (noiseSeed * 1103515245L + 12345L) & 0x7fffffffL;
                    value = ((noiseSeed / (double) 0x7fffffffL) * 2.0) - 1.0;
                } else {
                    value = Math.sin(phase);
                }
                data[i] = (byte) (value * envelope * volume * 127.0);
            }
            return data;
        }
    }
}
