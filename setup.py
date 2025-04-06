from setuptools import setup, Extension
setup(
    name='pyconfetti',
    version='0.1.0',
    description='Python package for the Confetti configuration language',
    long_description=open('README.md').read(),
    author='Confetti Contributors',
    url='https://github.com/hgs3/confetti',
    ext_modules=[Extension('pyconfetti', sources=['pyconfetti.c', 'confetti.c', 'confetti_unidata.c'])],
    include_package_data=True,
    zip_safe=False,
    classifiers=[
        'Development Status :: Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: Python :: 3.13',
    ],
)
