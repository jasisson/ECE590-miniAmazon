# -*- coding: utf-8 -*-
# Generated by Django 1.11 on 2017-05-03 20:14
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('polls', '0002_catalog_event_orders_user'),
    ]

    operations = [
        migrations.AddField(
            model_name='catalog',
            name='cat_type_num',
            field=models.IntegerField(default=0),
        ),
    ]