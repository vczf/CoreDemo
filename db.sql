
DROP TABLE IF EXISTS `compile_info`;
CREATE TABLE `compile_info` (
  `solution_id` int(11) NOT NULL,
  `error` text,
  `is_delete` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`solution_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `user_info`;
CREATE TABLE `user_info` (
  `user_name` varchar(40) NOT NULL,
  `password` varchar(40) DEFAULT NULL,
  `email` varchar(40) DEFAULT NULL,
  `nick` varchar(40) DEFAULT NULL,
  `student_id` varchar(20) DEFAULT NULL,
  `note` text,
  `register` datetime DEFAULT '2000-00-00 00:00:00',
  `role` int(11) DEFAULT '0',
  PRIMARY KEY (`user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `contest_info`;
CREATE TABLE `contest_info` (
  `contest_id` int(11) NOT NULL AUTO_INCREMENT,
  `title` varchar(100) NOT NULL,
  `description` text,
  `start_time` datetime DEFAULT '2000-00-00 00:00:00',
  `end_time` datetime DEFAULT '2000-00-00 00:00:00',
  `mode` int(11) DEFAULT '1',
  PRIMARY KEY (`contest_id`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `contest_player`;
CREATE TABLE `contest_player` (
  `contest_id` int(11) NOT NULL,
  `user_name` varchar(40) NOT NULL,
  PRIMARY KEY (`contest_id`,`user_name`),
  KEY `user_type` (`user_name`),
  CONSTRAINT `contest_player_ibfk_1` FOREIGN KEY (`contest_id`) REFERENCES `contest_info` (`contest_id`),
  CONSTRAINT `contest_player_ibfk_2` FOREIGN KEY (`user_name`) REFERENCES `user_info` (`user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;




DROP TABLE IF EXISTS `contest_problem`;
CREATE TABLE `contest_problem` (
  `problem_id` int(11) NOT NULL,
  `contest_id` int(11) NOT NULL,
  `title` varchar(100) NOT NULL DEFAULT '',
  `in_date` datetime DEFAULT NULL,
  PRIMARY KEY (`problem_id`,`contest_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `contest_rank_info`;
CREATE TABLE `contest_rank_info` (
  `contest_id` int(11) NOT NULL,
  `problem_id` int(11) NOT NULL,
  `user_name` varchar(40) NOT NULL DEFAULT '',
  `in_date` datetime DEFAULT NULL,
  `submit` int(11) DEFAULT '0',
  PRIMARY KEY (`contest_id`,`problem_id`,`user_name`),
  KEY `contest_user` (`contest_id`,`user_name`),
  CONSTRAINT `contest_rank_info_ibfk_1` FOREIGN KEY (`contest_id`, `user_name`) REFERENCES `contest_player` (`contest_id`, `user_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `problem_info`;
CREATE TABLE `problem_info` (
  `problem_id` int(11) NOT NULL AUTO_INCREMENT,
  `title` varchar(100) NOT NULL DEFAULT '',
  `description` text,
  `input` text,
  `output` text,
  `sample_input` text,
  `sample_output` text,
  `hint` text,
  `source` varchar(40) DEFAULT '',
  `time_limit` int(11) NOT NULL DEFAULT '0',
  `memory_limit` int(11) NOT NULL DEFAULT '0',
  `submit` int(11) NOT NULL DEFAULT '0',
  `solved` int(11) NOT NULL DEFAULT '0',
  `AC` int(11) NOT NULL DEFAULT '0',
  `PE` int(11) NOT NULL DEFAULT '0',
  `WA` int(11) NOT NULL DEFAULT '0',
  `RE` int(11) NOT NULL DEFAULT '0',
  `TLE` int(11) NOT NULL DEFAULT '0',
  `MLE` int(11) NOT NULL DEFAULT '0',
  `OLE` int(11) NOT NULL DEFAULT '0',
  `CE` int(11) NOT NULL DEFAULT '0',
  `hide` tinyint(1) NOT NULL DEFAULT '0',
  `is_delete` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`problem_id`)
) ENGINE=InnoDB AUTO_INCREMENT=1000 DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `solution_info`;
CREATE TABLE `solution_info` (
  `solution_id` int(11) NOT NULL AUTO_INCREMENT,
  `problem_id` int(11) NOT NULL DEFAULT '0',
  `user_name` varchar(40) NOT NULL,
  `run_time` int(11) NOT NULL DEFAULT '0',
  `run_memory` int(11) NOT NULL DEFAULT '0',
  `in_date` datetime NOT NULL DEFAULT '2000-00-00 00:00:00',
  `result` int(11) NOT NULL DEFAULT '0',
  `language` int(11) NOT NULL DEFAULT '0',
  `contest_id` int(11) DEFAULT '0',
  `code_length` int(11) NOT NULL DEFAULT '0',
  `is_delete` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`solution_id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=utf8;


DROP TABLE IF EXISTS `source_code`;
CREATE TABLE `source_code` (
  `solution_id` int(11) NOT NULL,
  `source` text NOT NULL,
  `is_delete` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`solution_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;




