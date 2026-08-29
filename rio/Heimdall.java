package com.heimdall;

import edu.wpi.first.networktables.BooleanSubscriber;
import edu.wpi.first.networktables.DoublePublisher;
import edu.wpi.first.networktables.DoubleSubscriber;
import edu.wpi.first.networktables.NetworkTable;
import edu.wpi.first.networktables.NetworkTableInstance;

/**
 * Copy this class into the robot project. No extra vendordep — it uses WPILib NT.
 *
 * periodic():
 *   heimdall.sendPose(pose.getX(), pose.getY(), pose.getRotation().getRadians());
 *   if (heimdall.hasThreat()) {
 *       drive.drive(heimdall.fleeX() * maxSpeed, heimdall.fleeY() * maxSpeed, 0.0);
 *   }
 */
public final class Heimdall implements AutoCloseable {
    private final DoublePublisher poseX;
    private final DoublePublisher poseY;
    private final DoublePublisher poseHeading;
    private final BooleanSubscriber hasThreat;
    private final DoubleSubscriber fleeX;
    private final DoubleSubscriber fleeY;
    private final DoubleSubscriber nearestRange;
    private final BooleanSubscriber healthy;

    public Heimdall() {
        NetworkTable t = NetworkTableInstance.getDefault().getTable("heimdall");
        poseX = t.getDoubleTopic("pose/x").publish();
        poseY = t.getDoubleTopic("pose/y").publish();
        poseHeading = t.getDoubleTopic("pose/heading").publish();
        hasThreat = t.getBooleanTopic("hasThreat").subscribe(false);
        fleeX = t.getDoubleTopic("fleeX").subscribe(0.0);
        fleeY = t.getDoubleTopic("fleeY").subscribe(0.0);
        nearestRange = t.getDoubleTopic("nearestRange").subscribe(0.0);
        healthy = t.getBooleanTopic("healthy").subscribe(false);
    }

    public void sendPose(double x, double y, double headingRad) {
        poseX.set(x);
        poseY.set(y);
        poseHeading.set(headingRad);
    }

    public boolean hasThreat() {
        return healthy.get() && hasThreat.get();
    }

    public double fleeX() {
        return fleeX.get();
    }

    public double fleeY() {
        return fleeY.get();
    }

    public double nearestRange() {
        return nearestRange.get();
    }

    public boolean isHealthy() {
        return healthy.get();
    }

    @Override
    public void close() {
        poseX.close();
        poseY.close();
        poseHeading.close();
        hasThreat.close();
        fleeX.close();
        fleeY.close();
        nearestRange.close();
        healthy.close();
    }
}
