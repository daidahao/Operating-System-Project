# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(alarm-priority-old) begin
(alarm-priority-old) Thread priority 30 woke up.
(alarm-priority-old) Thread priority 29 woke up.
(alarm-priority-old) Thread priority 28 woke up.
(alarm-priority-old) Thread priority 27 woke up.
(alarm-priority-old) Thread priority 26 woke up.
(alarm-priority-old) Thread priority 25 woke up.
(alarm-priority-old) Thread priority 24 woke up.
(alarm-priority-old) Thread priority 23 woke up.
(alarm-priority-old) Thread priority 22 woke up.
(alarm-priority-old) Thread priority 21 woke up.
(alarm-priority-old) end
EOF
pass;
