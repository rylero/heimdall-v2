package com.heimdall;

import edu.wpi.first.math.geometry.Translation2d;
import edu.wpi.first.networktables.BooleanSubscriber;
import edu.wpi.first.networktables.DoubleArraySubscriber;
import edu.wpi.first.networktables.DoublePublisher;
import edu.wpi.first.networktables.NetworkTable;
import edu.wpi.first.networktables.NetworkTableInstance;

/**
 * Copy this class into the robot project. No extra vendordep — it uses WPILib NT.
 *
 * periodic():
 *   heimdall.sendPose(pose.getX(), pose.getY(), pose.getRotation().getRadians());
 *   for (Translation2d other : heimdall.robots()) {
 *       // field-relative positions of other robots, meters
 *   }
 */
public final class Heimdall implements AutoCloseable {
    private final DoublePublisher poseX;
    private final DoublePublisher poseY;
    private final DoublePublisher poseHeading;
    private final DoubleArraySubscriber robotsX;
    private final DoubleArraySubscriber robotsY;
    private final BooleanSubscriber healthy;

    public Heimdall() {
        NetworkTable t = NetworkTableInstance.getDefault().getTable("heimdall");
        poseX = t.getDoubleTopic("pose/x").publish();
        poseY = t.getDoubleTopic("pose/y").publish();
        poseHeading = t.getDoubleTopic("pose/heading").publish();
        robotsX = t.getDoubleArrayTopic("robots/x").subscribe(new double[] {});
        robotsY = t.getDoubleArrayTopic("robots/y").subscribe(new double[] {});
        healthy = t.getBooleanTopic("healthy").subscribe(false);
    }

    public void sendPose(double x, double y, double headingRad) {
        poseX.set(x);
        poseY.set(y);
        poseHeading.set(headingRad);
    }

    /** Field-relative positions (meters) of other robots currently in view. Empty if none. */
    public Translation2d[] robots() {
        if (!healthy.get()) {
            return new Translation2d[0];
        }
        double[] xs = robotsX.get();
        double[] ys = robotsY.get();
        int n = Math.min(xs.length, ys.length);
        Translation2d[] out = new Translation2d[n];
        for (int i = 0; i < n; i++) {
            out[i] = new Translation2d(xs[i], ys[i]);
        }
        return out;
    }

    public boolean isHealthy() {
        return healthy.get();
    }

    @Override
    public void close() {
        poseX.close();
        poseY.close();
        poseHeading.close();
        robotsX.close();
        robotsY.close();
        healthy.close();
    }
}
