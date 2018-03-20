-- phpMyAdmin SQL Dump
-- version 4.7.9
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Mar 19, 2018 at 06:21 AM
-- Server version: 5.5.56-MariaDB
-- PHP Version: 5.6.34

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `cmarketcap_db`
--

-- --------------------------------------------------------

--
-- Table structure for table `coin_history`
--

CREATE TABLE `coin_history` (
  `index_` int(11) NOT NULL,
  `coin_key` varchar(60) NOT NULL,
  `min_0` int(11) NOT NULL DEFAULT '0',
  `min_5` int(11) NOT NULL DEFAULT '0',
  `min_10` int(11) NOT NULL DEFAULT '0',
`min_15` int(11) NOT NULL DEFAULT '0',
`min_20` int(11) NOT NULL DEFAULT '0',
`min_25` int(11) NOT NULL DEFAULT '0',
`min_30` int(11) NOT NULL DEFAULT '0',
`min_35` int(11) NOT NULL DEFAULT '0',
`min_40` int(11) NOT NULL DEFAULT '0',
`min_45` int(11) NOT NULL DEFAULT '0',
`min_50` int(11) NOT NULL DEFAULT '0',
`min_55` int(11) NOT NULL DEFAULT '0',
`hr_1` int(11) NOT NULL DEFAULT '0',
`hr_2` int(11) NOT NULL DEFAULT '0',
`hr_3` int(11) NOT NULL DEFAULT '0',
`hr_4` int(11) NOT NULL DEFAULT '0',
`hr_5` int(11) NOT NULL DEFAULT '0',
`hr_6` int(11) NOT NULL DEFAULT '0',
`hr_7` int(11) NOT NULL DEFAULT '0',
`hr_8` int(11) NOT NULL DEFAULT '0',
`hr_9` int(11) NOT NULL DEFAULT '0',
`hr_10` int(11) NOT NULL DEFAULT '0',
`hr_11` int(11) NOT NULL DEFAULT '0',
`hr_12` int(11) NOT NULL DEFAULT '0',
`hr_13` int(11) NOT NULL DEFAULT '0',
`hr_14` int(11) NOT NULL DEFAULT '0',
`hr_15` int(11) NOT NULL DEFAULT '0',
`hr_16` int(11) NOT NULL DEFAULT '0',
`hr_17` int(11) NOT NULL DEFAULT '0',
`hr_18` int(11) NOT NULL DEFAULT '0',
`hr_19` int(11) NOT NULL DEFAULT '0',
`hr_20` int(11) NOT NULL DEFAULT '0',
`hr_21` int(11) NOT NULL DEFAULT '0',
`hr_22` int(11) NOT NULL DEFAULT '0',
`hr_23` int(11) NOT NULL DEFAULT '0',
`hr_24` int(11) NOT NULL DEFAULT '0',
`day_1` int(11) NOT NULL DEFAULT '0',
`day_2` int(11) NOT NULL DEFAULT '0',
`day_3` int(11) NOT NULL DEFAULT '0',
`day_4` int(11) NOT NULL DEFAULT '0',
`day_5` int(11) NOT NULL DEFAULT '0',
`day_6` int(11) NOT NULL DEFAULT '0',
`day_7` int(11) NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `time_stamps`
--

CREATE TABLE `time_stamps` (
  `index_` int(11) NOT NULL,
  `column_name` varchar(20) NOT NULL,
  `time_stamp` int(11) NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


--
-- Indexes for dumped tables
--

--
-- Indexes for table `coin_history`
--
ALTER TABLE `coin_history`
  ADD PRIMARY KEY (`index_`),
  ADD UNIQUE KEY `coin_key` (`coin_key`);

--
-- Indexes for table `time_stamps`
--
ALTER TABLE `time_stamps`
  ADD PRIMARY KEY (`index_`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `coin_history`
--
ALTER TABLE `coin_history`
  MODIFY `index_` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT for table `time_stamps`
--
ALTER TABLE `time_stamps`
  MODIFY `index_` int(11) NOT NULL AUTO_INCREMENT;

--
-- Dumping data for table `time_stamps`
--

INSERT INTO `time_stamps` (`column_name`) VALUES
('min_0'),
('min_5'),
('min_10'),
('min_15'),
('min_20'),
('min_25'),
('min_30'),
('min_35'),
('min_40'),
('min_45'),
('min_50'),
('min_55'),
('hr_1'),
('hr_2'),
('hr_3'),
('hr_4'),
('hr_5'),
('hr_6'),
('hr_7'),
('hr_8'),
('hr_9'),
('hr_10'),
('hr_11'),
('hr_12'),
('hr_13'),
('hr_14'),
('hr_15'),
('hr_16'),
('hr_17'),
('hr_18'),
('hr_19'),
('hr_20'),
('hr_21'),
('hr_22'),
('hr_23'),
('hr_24'),
('day_1'),
('day_2'),
('day_3'),
('day_4'),
('day_5'),
('day_6'),
('day_7');

COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
