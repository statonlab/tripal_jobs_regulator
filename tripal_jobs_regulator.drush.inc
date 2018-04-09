<?php

/**
 * Implements hook_drush_command().
 */
function tripal_jobs_regulator_drush_command() {
  $commands = [];

  $commands['jobs-regulator-run'] = [
    'description' => dt('Launches jobs waiting in the queue. Only one job can execute at a time unless the --parallel=1 option is provided.'),
    'examples' => [
      'Single Job' => 'drush trp-run-jobs --username=administrator',
      'Parallel Job' => 'drush trp-run-jobs --username=administrator --parallel=1',
      'Max-jobs Job' => 'drush trp-run-jobs --username=administrator --parallel=1 --max_jobs=10',
      'Role' => 'drush trp-run-jobs --username=administrator --role=administrator',
    ],
    'arguments' => [],
    'options' => [
      'username' => [
        'description' => dt('The Drupal user name for which the job should be run.  The permissions for this user will be used.'),
      ],
      'parallel' => dt('Normally jobs are executed one at a time. But if you are certain no conflicts will occur with other currently running jobs you may set this argument to a value of 1 to make the job run in parallel with other running jobs.'),
      'max_jobs' => dt('Indicate the maximum number of concurrent jobs. Default is -1 (unlimited). Ignore if not running parallel jobs'),
      'single' => dt('Execute only one queued job'),
      'role' => dt('Execute jobs submitted by a given role. Drupal comes preset with "administrator", "authenticated user", and "anonymous user".'),
    ],
  ];

  return $commands;
}

/**
 * Implements drush_HOOK_COMMANDNAME()
 *
 * Runs when jobs-regulator-run is called
 */
function drush_tripal_jobs_regulator_jobs_regulator_run() {
  $parallel = drush_get_option('parallel', 0);
  $max_jobs = drush_get_option('max_jobs', -1);
  $single = drush_get_option('single', 0);
  $username = drush_get_option('username');
  $role = trim(drush_get_option('role', 'administrator'));

  // Check that the role exists
  $role_exists = db_query('SELECT * FROM {role} WHERE name=:name', [
    ':name' => $role,
  ])->fetchObject();

  if (!$role_exists) {
    drush_log('The specified role "' . $role . '" does not exist.', 'error');
  }

  // Set the user
  drush_tripal_set_user($username);

  // Run the jobs
  drush_print("\n" . date('Y-m-d H:i:s'));

  if ($parallel) {
    drush_print("Tripal Job Launcher (in parallel)");
    if ($max_jobs !== -1) {
      drush_print("Maximum number of jobs is " . $max_jobs);
    }
  }
  else {
    drush_print("Tripal Job Launcher");
  }

  $message = "Running as user '$username'. Role: $role";
  drush_print($message);
  drush_print(str_pad("", strlen($message), '-'));

  $count = tripal_jobs_regulator_count($role);
  drush_print("There are $count jobs pending for the given role.");

  while (($job_id = tripal_jobs_regulator_get_next_job($role)) !== FALSE) {
    tripal_launch_job($parallel, $job_id, $max_jobs, $single);
  }
}

/**
 * Get the next job to run.
 *
 * @param $role
 *
 * @return int|bool
 */
function tripal_jobs_regulator_get_next_job($role) {
  if ($role === 'anonymous user') {
    $sql = "
      SELECT TJ.job_id
      FROM tripal_jobs TJ
      WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND TJ.uid = 0
      ORDER BY priority ASC, job_id ASC
      LIMIT 1
    ";
    return db_query($sql)->fetchField() ?: FALSE;
  }

  if ($role === 'authenticated user') {
    $sql = "
      SELECT TJ.job_id
      FROM tripal_jobs TJ
      WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND TJ.uid > 1
      ORDER BY priority ASC, job_id ASC
      LIMIT 1
    ";

    return db_query($sql)->fetchField() ?: FALSE;
  }


  $sql = "
      SELECT TJ.job_id
      FROM tripal_jobs TJ
      INNER JOIN users U ON TJ.uid = U.uid
      INNER JOIN users_roles UR ON U.uid = UR.uid
      INNER JOIN role R ON R.rid = UR.rid    
      WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND R.name=:role
      ORDER BY priority ASC, job_id ASC
      LIMIT 1
    ";

  return db_query($sql, [':role' => $role])->fetchField() ?: FALSE;
}

/**
 * Count the number of available jobs to run.
 *
 * @return mixed
 */
function tripal_jobs_regulator_count($role) {
  if ($role === 'anonymous user') {
    return db_query("
        SELECT COUNT(TJ.job_id)
        FROM tripal_jobs TJ
        WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND TJ.uid = 0
    ")->fetchField();
  }

  if ($role === 'authenticated user') {
    return db_query("
        SELECT COUNT(TJ.job_id)
        FROM tripal_jobs TJ
        WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND TJ.uid > 1
    ")->fetchField();
  }

  $query = "SELECT COUNT(TJ.job_id)
      FROM tripal_jobs TJ
      INNER JOIN users U ON TJ.uid = U.uid
      INNER JOIN users_roles UR ON U.uid = UR.uid
      INNER JOIN role R ON R.rid = UR.rid    
      WHERE TJ.start_time IS NULL
        AND TJ.end_time IS NULL
        AND TJ.status != 'Cancelled'
        AND R.name=:role";

  return db_query($query, [':role' => $role])->fetchField();
}
