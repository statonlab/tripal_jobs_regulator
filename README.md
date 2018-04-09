This simple module is a wrapper around Tripal jobs to allow for an option to specify
the role of the job submitter. Therefore, making it possible to give each role an
individual job runner entry in cron.

### Installation
```bash
# From drupal root (usually /var/www/html)
cd sites/all/modules
git clone https://github.com/statonlab/tripal_jobs_regulator.git
drush en tripal_jobs_regulator
```

Once enabled, a new drush command `jobs-regulator-run` should become accessible.

### Usage
Example usage with all available options:
```bash
drush jobs-regulator-run --username="administrator" --role="authenticated user" --parallel=1 --max_jobs=3 --root=/var/www/html
```

**Options**

| Name | Description |
| ---- | ----------- |
| `username` | **REQUIRED**. The user to run the jobs |
| `role` | **REQUIRED**. The role of the job submitter |
| `parallel` | **OPTIONAL**. Whether to run jobs in parallel |
| `max_jobs` | **OPTIONAL**. When running in parallel, how many jobs should run simultaneously. Use -1 or leave blank if all jobs should run in parallel without restrictions |

