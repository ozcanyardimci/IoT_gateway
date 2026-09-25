#include <unity.h>
#include "mqtt_outbox.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_interior_failure_compaction(void) {
    MqttOutbox outbox;
    
    // Fill 3 messages
    outbox.enqueue("t1", "p1");
    outbox.enqueue("t2", "p2");
    outbox.enqueue("t3", "p3");
    
    // Simulate flushOutbox where t1 succeeds, t2 fails (so t2, t3 stay)
    MqttOutboxItem* i1 = outbox.getItem(0);
    MqttOutboxItem* i2 = outbox.getItem(1);
    MqttOutboxItem* i3 = outbox.getItem(2);
    
    TEST_ASSERT_EQUAL_STRING("t1", i1->topic);
    TEST_ASSERT_EQUAL_STRING("t2", i2->topic);
    TEST_ASSERT_EQUAL_STRING("t3", i3->topic);
    
    i1->used = false; // t1 sent successfully
    // t2 failed, so it and t3 remain used.
    
    outbox.compact();
    
    // Now index 0 should be t2, index 1 should be t3
    MqttOutboxItem* new_i0 = outbox.getItem(0);
    MqttOutboxItem* new_i1 = outbox.getItem(1);
    
    TEST_ASSERT_TRUE(new_i0->used);
    TEST_ASSERT_EQUAL_STRING("t2", new_i0->topic);
    
    TEST_ASSERT_TRUE(new_i1->used);
    TEST_ASSERT_EQUAL_STRING("t3", new_i1->topic);
    
    // Index 2 should be unused
    TEST_ASSERT_FALSE(outbox.getItem(2)->used);
}

void test_backfill_after_compaction(void) {
    MqttOutbox outbox;
    outbox.enqueue("t1", "p1");
    outbox.enqueue("t2", "p2");
    
    // t1 sent, compact
    outbox.getItem(0)->used = false;
    outbox.compact();
    
    // Enqueue a new one, should go to index 1
    outbox.enqueue("t3", "p3");
    
    TEST_ASSERT_TRUE(outbox.getItem(0)->used);
    TEST_ASSERT_EQUAL_STRING("t2", outbox.getItem(0)->topic);
    
    TEST_ASSERT_TRUE(outbox.getItem(1)->used);
    TEST_ASSERT_EQUAL_STRING("t3", outbox.getItem(1)->topic);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_interior_failure_compaction);
    RUN_TEST(test_backfill_after_compaction);
    return UNITY_END();
}
