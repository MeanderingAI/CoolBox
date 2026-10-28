package io.coolbox.bandit;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class BanditTest {@Test void pullsAndTracksRewards(){try(BanditArm arm=new BanditArm(1)){assertEquals(1,arm.pull());arm.update(1);assertEquals(1,arm.pullCount());assertEquals(1,arm.estimatedProbability());}}@Test void rejectsUseAfterClose(){BanditArm arm=new BanditArm(.5);arm.close();assertThrows(IllegalStateException.class,arm::pull);}}
