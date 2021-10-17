package com.romkas.mandelbrot;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.event.*;
import java.io.File;
import java.io.IOException;
import java.util.function.Consumer;

import javax.imageio.ImageIO;
import javax.swing.SwingUtilities;
import javax.swing.event.MouseInputListener;

/**
 *
 * @author gramelis
 */
public class GUI extends javax.swing.JFrame
{
    private static class ImageGeneratorThread extends Thread
    {
        private int width;
        private int height;
        private int threads;
        private int supersample;
        private DoublePoint center;
        private double diameter;
        private String file;

        Consumer<Image> onSuccess;
        Runnable onFailure;

        public ImageGeneratorThread(int width, int height, DoublePoint center,
                int threads, int ss, double diameter, String file,
                Consumer<Image> onSuccess,
                Runnable onFailure)
        {
            this.width = width;
            this.height = height;
            this.center = new DoublePoint(center.x, center.y);
            this.diameter = diameter;
            this.supersample = ss;
            this.file = file;
            this.threads = threads;

            this.onSuccess = onSuccess;
            this.onFailure = onFailure;
        }

        public void run()
        {
            String[] args = new String[] {
                "mandelbrot",
                    "-t", Integer.toString(threads),
                    "-f", file,
                    "-w", Integer.toString(width),
                    "-h", Integer.toString(height),
                    "-x", Double.toString(center.x),
                    "-y", Double.toString(center.y),
                    "-r", Double.toString(diameter),
                    "-s", Integer.toString(supersample)
            };
            try {
                Process p = Runtime.getRuntime().exec(args);
                while (true) {
                    try {
                        p.waitFor();
                        break;
                    } catch (InterruptedException ex) {
                        if (!p.isAlive())
                            break;
                    }
                }
                Image img = ImageIO.read(new File(file));
                onSuccess.accept(img);
            } catch (IOException ex) {
                System.err.println(ex.getMessage());
                onFailure.run();
            }
        }
    }

    private IntPoint firstClickPoint = null;
    private IntPoint lastClickPoint = null;
    private Image img = null;

    private int supersampleLevel = 1;
    private DoublePoint center = new DoublePoint(0.0, 0.0);
    private double diameter = 1.5;
    private int threadCount = Runtime.getRuntime().availableProcessors();
    private String bitmapFile = "./__bitmap.bmp";
    private boolean drawingImage = false;

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private java.awt.Canvas canvas1;
    // End of variables declaration//GEN-END:variables

    public GUI() {
        initComponents();
    }

    @SuppressWarnings("unchecked")
    private void initComponents() {

        canvas1 = new java.awt.Canvas();

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);
        setTitle("Mandelbrot viewer");
        setMinimumSize(new java.awt.Dimension(640, 480));
        setPreferredSize(new java.awt.Dimension(1280, 768));

        canvas1.setName("canvas"); // NOI18N
        canvas1.addMouseListener(new MouseInputListener()
        {
            @Override
            public void mouseExited(MouseEvent evt) {}
            @Override
            public void mouseClicked(MouseEvent evt) {}
            @Override
            public void mousePressed(MouseEvent evt)
            {
                canvasMousePressed(evt);
            }
            @Override
            public void mouseMoved(MouseEvent evt) {}
            @Override
            public void mouseEntered(MouseEvent evt) {}
            @Override
            public void mouseDragged(MouseEvent evt) {}
            @Override
            public void mouseReleased(MouseEvent evt)
            {
                canvasMouseReleased(evt);
            }
        });

        canvas1.addComponentListener(new ComponentListener()
        {
            @Override
            public void componentMoved(ComponentEvent evt) {}
            @Override
            public void componentHidden(ComponentEvent evt) {}
            @Override
            public void componentShown(ComponentEvent evt) {}
            @Override
            public void componentResized(ComponentEvent evt)
            {
                drawImage();
            }
        });

        javax.swing.GroupLayout layout = new javax.swing.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(canvas1, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                .addContainerGap())
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
            .addGroup(layout.createSequentialGroup()
                .addContainerGap()
                .addComponent(canvas1, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, Short.MAX_VALUE)
                .addContainerGap())
        );

        pack();
        drawImage();
    }// </editor-fold>//GEN-END:initComponents

    private void drawImage()
    {
        if (drawingImage) {
            return;
        }
        drawingImage = true;
        new ImageGeneratorThread(
            canvas1.getWidth(), canvas1.getHeight(), new DoublePoint(center.x, center.y),
            threadCount, supersampleLevel, diameter, bitmapFile,
            (Image img) ->
            {
                SwingUtilities.invokeLater(() ->
                {
                    canvas1.setSize(img.getWidth(null), img.getHeight(null));
                    Graphics g = canvas1.getGraphics();
                    g.drawImage(img, 0, 0, null);
                    drawingImage = false;
                });
            },
            () ->
            {
                SwingUtilities.invokeLater(() ->
                {
                    Graphics g = canvas1.getGraphics();
                    g.drawString("FUCK", 0, 0);
                    SwingUtilities.invokeLater(() -> {
                        System.exit(1);
                    });
                });
            }
        ).start();
    }

    private void canvasMousePressed(MouseEvent evt)
    {
        if (drawingImage) {
            return;
        }
        firstClickPoint = new IntPoint(evt.getX(), evt.getY());
    }

    private void canvasMouseReleased(MouseEvent evt)
    {
        if (drawingImage) {
            return;
        }

        lastClickPoint = new IntPoint(evt.getX(), evt.getY());

        int canvasWidth = canvas1.getWidth();
        int canvasHeight = canvas1.getHeight();
        double pixelWidth = diameter / Math.min(canvasHeight, canvasWidth);
        int minX = Math.min(firstClickPoint.x, lastClickPoint.x);
        int minY = Math.min(firstClickPoint.y, lastClickPoint.y);
        int dx = Math.abs(firstClickPoint.x - lastClickPoint.x);
        int dy = Math.abs(firstClickPoint.y - lastClickPoint.y);
        int cx = minX + dx / 2;
        int cy = minY + dy / 2;
        diameter = Math.min(dx, dy) * pixelWidth;

        int xCenterOffset = cx - (canvasWidth / 2);
        int yCenterOffset = cy - (canvasHeight / 2);

        center = new DoublePoint(
                center.x + xCenterOffset * pixelWidth,
                center.y - yCenterOffset * pixelWidth);

        firstClickPoint = null;
        lastClickPoint = null;

        drawImage();
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        /* Set the Nimbus look and feel */
        //<editor-fold defaultstate="collapsed" desc=" Look and feel setting code (optional) ">
        /* If Nimbus (introduced in Java SE 6) is not available, stay with the default look and feel.
         * For details see http://download.oracle.com/javase/tutorial/uiswing/lookandfeel/plaf.html 
         */
        try {
            for (javax.swing.UIManager.LookAndFeelInfo info : javax.swing.UIManager.getInstalledLookAndFeels()) {
                if ("Nimbus".equals(info.getName())) {
                    javax.swing.UIManager.setLookAndFeel(info.getClassName());
                    break;
                }
            }
        } catch (ClassNotFoundException ex) {
            java.util.logging.Logger.getLogger(GUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (InstantiationException ex) {
            java.util.logging.Logger.getLogger(GUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (IllegalAccessException ex) {
            java.util.logging.Logger.getLogger(GUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (javax.swing.UnsupportedLookAndFeelException ex) {
            java.util.logging.Logger.getLogger(GUI.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        }
        //</editor-fold>

        /* Create and display the form */
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                new GUI().setVisible(true);
            }
        });
    }
}
