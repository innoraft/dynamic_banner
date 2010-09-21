<?php

  // $Id$

  /**
   * @file
   * This file will determine how the dynamic banner block will display
   */

  // input variables need to have the same name as the theme definitions

  // Url
  $url;
  // Text (optional)
  $text;
  // display setting
  $display_setting;
  // display errors
  $display_errors;

  // default the output
  $output = "";

  // decide how to output based on what the displaysetting is
  if ($display_setting == 'urltext') {
    // check for variables
    if ($url && $text) {
      // form the html snippet and print
      $output .= "<div id='banner-left'>";// match this up with your css
      $output .= "<img src='/$url' class='jpg' alt='banner' width='596' height='209' />";
      $output .= "</div><div id='banner-right'>";// if you have text in your banner match this up with your css
      $output .= "<p id='bannerTitle'>" . $text . "</p>";
      $output .= "</div>";
      echo $output;
    }
    elseif ( $display_errors) {
      echo "problem";
    }
  }
  elseif ($display_setting == 'url') {
    // just an image url came in
    if ($url) {
      $output .= "<div id='banner'>";
      $output .= "<img src='/$url' class='jpg' alt='banner' width='596' height='209' />";
      $output .= "</div>";
      echo $output;
    }
    elseif ( $display_errors) {
      echo "problem";
    }
  }
  elseif ($display_setting == 'text') {
    // just text came in
    if ($url) {
      $output .= "<div id='banner'>";
      $output .= "<p>$text</p>";
      $output .= "</div>";
      echo $output;
    }
    elseif ( $display_errors) {
      echo "problem";
    }
  }