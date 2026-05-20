#include <gtest/gtest.h>
#include "StateManager.h"
#include <stdexcept>

TEST(FSM, InitialStateIsIdle) {
    StateManager sm;
    EXPECT_EQ(sm.state(), DaemonState::IDLE);
}

TEST(FSM, StartTransitionsToRunning) {
    StateManager sm;
    sm.start();
    EXPECT_EQ(sm.state(), DaemonState::RUNNING);
}

TEST(FSM, HaltFromRunningReturnsIdle) {
    StateManager sm;
    sm.start();
    sm.halt();
    EXPECT_EQ(sm.state(), DaemonState::IDLE);
}

TEST(FSM, FaultFromRunning) {
    StateManager sm;
    sm.start();
    sm.fault("s", 99.0, 80.0);
    EXPECT_EQ(sm.state(), DaemonState::FAULT);
}

TEST(FSM, StartFromFaultResumesRunning) {
    StateManager sm;
    sm.start();
    sm.fault("s", 99.0, 80.0);
    sm.start();
    EXPECT_EQ(sm.state(), DaemonState::RUNNING);
}

TEST(FSM, HaltFromIdleIsNoOp) {
    StateManager sm;
    sm.halt();
    EXPECT_EQ(sm.state(), DaemonState::IDLE);
}

TEST(FSM, StartFromRunningThrows) {
    StateManager sm;
    sm.start();
    EXPECT_THROW(sm.start(), std::logic_error);
}

TEST(FSM, StateChangedCallbackFired) {
    StateManager sm;
    DaemonState captured = DaemonState::SHUTDOWN;
    sm.setStateChangedCallback([&](DaemonState n, DaemonState) { captured = n; });
    sm.start();
    EXPECT_EQ(captured, DaemonState::RUNNING);
}

TEST(FSM, AlarmCallbackFired) {
    StateManager sm;
    sm.start();
    std::string sid;
    double val = 0.0, thr = 0.0;
    sm.setAlarmCallback([&](const std::string& s, double v, double t) {
        sid = s; val = v; thr = t;
    });
    sm.fault("sensor1", 99.5, 80.0);
    EXPECT_EQ(sid, "sensor1");
    EXPECT_DOUBLE_EQ(val, 99.5);
    EXPECT_DOUBLE_EQ(thr, 80.0);
}
